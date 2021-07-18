#include "console.hpp"

#include "font.hpp"
#include <cstring>

Console::Console(
    PixelWriter& writer,
    const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color},
      buffer_{}, cursor_row_{0}, cursor_column_{0} {
}

void Console::PutString(const char* s) {
    while (*s) {
        if (*s == '\n') {
            Newline();
        } else if (cursor_column_ < kColumns - 1) {
            WriteAscii(writer_, kColumnWidth * cursor_column_, kRowHight * cursor_row_, *s, fg_color_);
            buffer_[cursor_row_][cursor_column_] = *s;
            ++cursor_column_; // TODO: wrap and scroll
        }
        ++s;
    }
}

void Console::Newline() {
    cursor_column_ = 0;
    if (cursor_row_ < kRows - 1) {
        ++cursor_row_;
        return;
    }

    // clear console
    Clear();

    // scroll
    for (int row = 0; row < kRows - 1; ++row) {
        // shift buffer by one line
        memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
        // redraw
        WriteString(writer_, 0, 16 * row, buffer_[row], fg_color_);
    }
    memset(buffer_[kRows - 1], 0, kColumns); // fill zero to last line buffer
}

void Console::Clear() {
    for (int y = 0; y < kRowHight * kRows; ++y) {
        for (int x = 0; x < kColumnWidth * kColumns; ++x) {
            writer_.Write(x, y, bg_color_);
        }
    }
}