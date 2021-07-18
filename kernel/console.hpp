#pragma once

#include "graphics.hpp"

class Console {
public:
    static const int kRows = 25, kColumns = 80, kRowHight = 16, kColumnWidth = 8;

    Console(
        PixelWriter& writer,
        const PixelColor& fg_color, const PixelColor& bg_color);

    void PutString(const char* s);
    void Clear();

private:
    void Newline();

    PixelWriter& writer_;
    const PixelColor fg_color_, bg_color_;
    char buffer_[kRows][kColumns + 1];
    int cursor_row_, cursor_column_;
};
