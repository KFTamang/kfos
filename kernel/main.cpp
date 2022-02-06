#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>

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
#include "interrupt.hpp"
#include "queue.hpp"
#include "asmfunc.h"
#include "memory_map.hpp"

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

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

usb::xhci::Controller *xhc;

// #@@range_begin(queue_message)
struct Message
{
    enum Type
    {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message> *main_queue;
__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame)
{
    main_queue->Push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}

extern "C" void KernelMain(const FrameBufferConfig &frame_buffer_config, const MemoryMap &memmap)
{
    SetLogLevel(kError);

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

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;

    FillRectangle(*pixel_writer,
                  {0, 0},
                  {kFrameWidth, kFrameHeight - 50},
                  kDesktopBGColor);
    FillRectangle(*pixel_writer,
                  {0, kFrameHeight - 50},
                  {kFrameWidth, 50},
                  {1, 8, 17});
    FillRectangle(*pixel_writer,
                  {0, kFrameHeight - 50},
                  {kFrameWidth / 5, 50},
                  {80, 80, 80});
    DrawRectangle(*pixel_writer,
                  {10, kFrameHeight - 40},
                  {30, 30},
                  {160, 160, 160});

    // allocate global console for printk
    console = new (console_buf) Console(*pixel_writer, kDesktopFGColor, kDesktopBGColor);

    printk("Welcome to KFOS!\n");

    mouse_cursor = new (mouse_cursor_buf) MouseCursor{
        pixel_writer, kDesktopBGColor, {300, 200}};

    std::array<Message, 32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

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

    const uint16_t cs = GetCS();
    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    const uint8_t bsp_local_apic_id =
        *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
    pci::ConfigureMSIFixedDestination(
        *xhc_dev, bsp_local_apic_id,
        pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed,
        InterruptVector::kXHCI, 0);

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

    ::xhc = &xhc;

    usb::HIDMouseDriver::default_observer = MouseObserver;

    for (int i = 1; i <= xhc.MaxPorts(); ++i)
    {
        auto port = xhc.PortAt(i);
        Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

        if (port.IsConnected())
        {
            if (auto err = ConfigurePort(xhc, port))
            {
                Log(kError, "failed to configure port: %s at %s:%d\n",
                    err.Name(), err.File(), err.Line());
                continue;
            }
        }
    }

    while (1)
    {
        // #@@range_begin(get_front_message)
        __asm__("cli");
        if (main_queue.Count() == 0)
        {
            __asm__("sti\n\thlt");
            continue;
        }

        Message msg = main_queue.Front();
        main_queue.Pop();
        __asm__("sti");
        // #@@range_end(get_front_message)

        switch (msg.type)
        {
        case Message::kInterruptXHCI:
            while (xhc.PrimaryEventRing()->HasFront())
            {
                if (auto err = ProcessEvent(xhc))
                {
                    Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
                        err.Name(), err.File(), err.Line());
                }
            }
            break;
        default:
            Log(kError, "Unknown message type: %d\n", msg.type);
        }
    }
}

extern "C" void __cxa_pure_virtual()
{
    while (1)
        __asm__("hlt");
}
