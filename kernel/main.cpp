#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "font.hpp"
#include "graphics.hpp"

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
    for (int x = 0; x < 200; x++)
    {
        for (int y = 0; y < 100; y++)
        {
            pixel_writer->Write(x, y, {0, 255, 128});
        }
    }

    for (unsigned char c = 0; c < 64; c++)
    {
        WriteAscii(*pixel_writer, 8 * c, 100, c, {0, 0, 0});
        WriteAscii(*pixel_writer, 8 * c, 116, c + 64, {0, 0, 0});
    }
    char buf[256];
    sprintf(buf, "1 + 2 = %d", 1 + 2);
    WriteString(*pixel_writer, 50, 132, "Hello, world!", {0, 0, 0});
    WriteString(*pixel_writer, 50, 148, buf, {0, 0, 0});

    while (1)
    {
        __asm__("hlt");
    }
}
