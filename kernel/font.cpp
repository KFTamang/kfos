#include "font.hpp"

void WriteAscii(PixelWriter &pixel_writer, int x, int y, char c, const PixelColor &color)
{
    if (c != 'A')
    {
        return;
    }
    for (int dy = 0; dy < 16; dy++)
    {
        for (int dx = 0; dx < 8; dx++)
        {
            if ((kfontA[dy] << dx) & 0x80u)
            {
                pixel_writer.Write(x + dx, y + dy, color);
            }
        }
    }
}
