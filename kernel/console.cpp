#include "console.hpp"
#include "font.hpp"
#include <cstring>

Console::Console(PixelWriter &writer, const PixelColor &fg_color, const PixelColor &bg_color) : writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color}, buf{}, cursor_row_(0), cursor_column_(0)
{
}

void Console::PutString(const char *string)
{
    while (*string)
    {
        if (*string == '\n')
        {
            NewLine();
        }
        else
        {
            if (cursor_column_ < kColumns)
            {
                buf[cursor_row_][cursor_column_] = *string;
                WriteAscii(writer_, 8 * cursor_column_, 16 * cursor_row_, *string, fg_color_);
                cursor_column_++;
            }
        }
        string++;
    }
}

void Console::NewLine()
{
    cursor_column_ = 0;
    if (cursor_row_ < kRows - 1)
    {
        cursor_row_++;
    }
    else
    {
        for (int x = 0; x < 8 * kColumns; x++)
        {
            for (int y = 0; y < 16 * kRows; y++)
            {
                writer_.Write(x, y, bg_color_);
            }
        }
        for (int row = 0; row < kRows - 1; row++)
        {
            memcpy(buf[row], buf[row + 1], kColumns + 1);
            WriteString(writer_, 0, 16 * row, buf[row], fg_color_);
        }
        memset(buf[kRows - 1], 0, kColumns + 1);
    }
}