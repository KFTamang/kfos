#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "font.hpp"
#include "graphics.hpp"
#include "console.hpp"
#include "Error.hpp"
#include "pci.hpp"

// pixel drawer definition
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

// console definition
char console_buf[sizeof(Console)];
Console *console;

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

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@...@@..@      ",
    "@..@  @..@     ",
    "@.@   @..@     ",
    "@@     @..@    ",
    "@      @..@    ",
    "        @..@   ",
    "         @@@   "};

void DrawMouseCursor(int x, int y)
{
    for (int yy = 0; yy < kMouseCursorHeight; yy++)
    {
        for (int xx = 0; xx < kMouseCursorWidth; xx++)
        {
            if (mouse_cursor_shape[yy][xx] == '@')
            {
                pixel_writer->Write(x + xx, y + yy, {0, 0, 0});
            }
            else if (mouse_cursor_shape[yy][xx] == '.')
            {
                pixel_writer->Write(x + xx, y + yy, {255, 255, 255});
            }
        }
    }
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

    for (int i = 0; i < 20; i++)
    {
        const auto &dev = pci::devices[i];
        auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
        auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
        printk("%d.%d.%d: vend %04x, class %08x, head %02x\n",
               dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
    }

    DrawMouseCursor(100, 200);

    while (1)
    {
        __asm__("hlt");
    }
}
