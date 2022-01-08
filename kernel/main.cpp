#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "font.hpp"
#include "graphics.hpp"
#include "console.hpp"

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

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

    char buf[255];
    Console console(*pixel_writer, {255, 255, 255}, {10, 10, 10});
    for (int i = 0; i < 30; i++)
    {
        sprintf(buf, "line %d\n", i);
        console.PutString(buf);
    }

    while (1)
    {
        __asm__("hlt");
    }
}
