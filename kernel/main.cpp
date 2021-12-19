#include <cstdint>
#include "frame_buffer_config.hpp"

struct PixelColor{
   uint8_t r, g, b;
};


int WritePixel(const FrameBufferConfig& config, int x, int y, const PixelColor& c)
{
    const int pixel_position = config.pixels_per_scan_line * y + x;
    if(config.pixel_format == kPixelBGRResv8BitPerColor)
    {
      uint8_t* p = &config.frame_buffer[4 * pixel_position];
      p[0] = c.r;
      p[1] = c.g;
      p[2] = c.b;
    }else if(config.pixel_format == kPixelBGRResv8BitPerColor)
    {
      uint8_t* p = &config.frame_buffer[4 * pixel_position];
      p[0] = c.b;
      p[1] = c.g;
      p[2] = c.r;
    }else{
      return -1;
    }
    return 0;
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config)
{
    for(uint64_t i = 0; i < frame_buffer_config.horizontal_resolution; i++)
    {
        for(uint64_t j = 0; j < frame_buffer_config.vertical_resolution; j++)
        {
            WritePixel(frame_buffer_config, i, j, {255, 255, 255});
        }
    }
    for(int x = 0; x < 200; x++)
    {
        for(int y = 0; y < 100; y++)
        {
            WritePixel(frame_buffer_config, x, y, {0, 0, 0});
        }
    }

    while(1)
    {
        __asm__("hlt");
    }
}

