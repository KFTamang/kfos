#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include <cstdint>
#include <cstddef>

void *operator new(size_t size, void *buf) noexcept
{
    return buf;
}

void operator delete(void *obj) noexcept
{
}

uint8_t *PixelWriter::PixelAt(int x, int y)
{
    return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
}

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor &c)
{
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
}
