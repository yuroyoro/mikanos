#pragma once

#include <memory>
#include "graphics.hpp"
#include "window.hpp"

class Console {
public:
  static const int kRows = 25, kColumns = 80, kRowHight = 16, kColumnWidth = 8;

  Console(const PixelColor& fg_color, const PixelColor& bg_color);

  void PutString(const char* s);
  void SetWriter(PixelWriter* writer);
  void SetWindow(const std::shared_ptr<Window>& window);
  void Clear();

private:
  void Newline();
  void Refresh();

  PixelWriter* writer_;
  std::shared_ptr<Window> window_;
  const PixelColor fg_color_, bg_color_;
  char buffer_[kRows][kColumns + 1];
  int cursor_row_, cursor_column_;
};
