/**
 * @file console.cpp
 *
 * コンソール描画のプログラムを集めたファイル．
 */

#include "console.hpp"

#include <cstring>
#include "font.hpp"
#include "layer.hpp"

Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
    : writer_{nullptr}, window_{}, fg_color_{fg_color}, bg_color_{bg_color}, buffer_{}, cursor_row_{0}, cursor_column_{0} {
}

void Console::PutString(const char* s) {
  while (*s) {
    if (*s == '\n') {
      Newline();
    } else if (cursor_column_ < kColumns - 1) {
      WriteAscii(*writer_, Vector2D<int>{kColumnWidth * cursor_column_, kRowHight * cursor_row_}, *s, fg_color_);
      buffer_[cursor_row_][cursor_column_] = *s;
      ++cursor_column_;  // TODO: wrap and scroll
    }
    ++s;
  }
  if (layer_manager) {
    layer_manager->Draw();
  }
}

void Console::SetWriter(PixelWriter* writer) {
  if (writer == writer_) {
    return;
  }
  writer_ = writer;
  window_.reset();
  Refresh();
}

void Console::SetWindow(const std::shared_ptr<Window>& window) {
  if (window == window_) {
    return;
  }

  window_ = window;
  writer_ = window->Writer();
  Refresh();
}

void Console::Newline() {
  cursor_column_ = 0;
  if (cursor_row_ < kRows - 1) {
    ++cursor_row_;
    return;
  }

  if (window_) {
    Rectangle<int> move_src{{0, kRowHight}, {kColumnWidth * kColumns, kRowHight * (kRows - 1)}};
    window_->Move({0, 0}, move_src);
    FillRectangle(*writer_, {0, kRowHight * (kRows - 1)}, {kColumnWidth * kColumns, kRowHight}, bg_color_);
    return;
  }

  // clear console
  Clear();

  // scroll
  for (int row = 0; row < kRows - 1; ++row) {
    // shift buffer by one line
    memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
    // redraw
    WriteString(*writer_, Vector2D<int>{0, kRowHight * row}, buffer_[row], fg_color_);
  }
  memset(buffer_[kRows - 1], 0, kColumns);  // fill zero to last line buffer
}

void Console::Clear() {
  FillRectangle(*writer_, {0, 0}, {kColumnWidth * kColumns, kRowHight * kRows}, bg_color_);
}

void Console::Refresh() {
  for (int row = 0; row < kRows; ++row) {
    WriteString(*writer_, Vector2D<int>{0, kRowHight * row}, buffer_[row], fg_color_);
  }
}