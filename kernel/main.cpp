#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "font.hpp"
#include "graphics.hpp"
#include "console.hpp"
#include "error.hpp"
#include "pci.hpp"
#include "mouse.hpp"
#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

// pixel drawer definition
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

// console definition
char console_buf[sizeof(Console)];
Console *console;

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

// printk function for debug
void printk(const char *format, ...)
{
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);
    console->PutString(s);
}

void MouseObserver(int8_t displacement_x, int8_t displacement_y)
{
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
}
// #@@range_end(mouse_observer)

// #@@range_begin(switch_echi2xhci)
void SwitchEhci2Xhci(const pci::Device &xhc_dev)
{
    bool intel_ehc_exist = false;
    for (int i = 0; i < pci::num_device; ++i)
    {
        if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
            0x8086 == pci::ReadVendorId(pci::devices[i]))
        {
            intel_ehc_exist = true;
            break;
        }
    }
    if (!intel_ehc_exist)
    {
        return;
    }

    uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc); // USB3PRM
    pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);          // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);  // XUSB2PRM
    pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);           // XUSB2PR
    Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
        superspeed_ports, ehci2xhci_ports);
}

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config)
{
    switch (frame_buffer_config.pixel_format)
    {
    case kPixelRGBResv8BitPerColor:
        pixel_writer = new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frame_buffer_config};
        break;

    case kPixelBGRResv8BitPerColor:
        pixel_writer = new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frame_buffer_config};
        break;

    default:
        break;
    }
    for (uint64_t i = 0; i < frame_buffer_config.horizontal_resolution; i++)
    {
        for (uint64_t j = 0; j < frame_buffer_config.vertical_resolution; j++)
        {
            pixel_writer->Write(i, j, {255, 255, 255});
        }
    }
    for (int x = 0; x < 8 * Console::kColumns; x++)
    {
        for (int y = 0; y < 16 * Console::kRows; y++)
        {
            pixel_writer->Write(x, y, {0, 0, 0});
        }
    }

    // allocate global console for printk
    console = new (console_buf) Console(*pixel_writer, {255, 255, 255}, {10, 10, 10});
    for (int i = 0; i < 30; i++)
    {
        printk("printk line %d\n", i);
    }

    printk("Welcome to KFOS!\n");

    // List all pci devices
    auto err = pci::ScanAllBus();
    printk("ScanAllBus: %s\n", err.Name());

    // search for xhc device
    pci::Device *xhc_dev = nullptr;
    for (int i = 0; i < pci::num_device; i++)
    {
        if (pci::devices[i].class_code.Match(0x0cu, 0x3u, 0x30u))
        {
            xhc_dev = &pci::devices[i];
            if (0x8086 == pci::ReadVendorId(*xhc_dev))
            {
                break;
            }
        }
    }
    printk("xHC device found. %d.%d.%d\n",
           xhc_dev->bus, xhc_dev->device, xhc_dev->function);

    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    usb::xhci::Controller xhc{xhc_mmio_base};

    if (0x8086 == pci::ReadVendorId(*xhc_dev))
    {
        SwitchEhci2Xhci(*xhc_dev);
    }
    {
        auto err = xhc.Initialize();
        Log(kDebug, "xhc.Initialize() : %s\n", err.Name());
    }

    Log(kDebug, "xHC starting\n");
    xhc.Run();

    while (1)
    {
        __asm__("hlt");
    }
}

extern "C" void __cxa_pure_virtual()
{
    while (1)
        __asm__("hlt");
}
