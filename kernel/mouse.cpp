#include "mouse.hpp"

#include "graphics.hpp"

namespace {
  const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
      "@              ",
      "@@             ",
      "@.@            ",
      "@..@           ",
      "@...@          ",
      "@....@         ",
      "@.....@        ",
      "@......@       ",
      "@.......@      ",
      "@........@     ",
      "@.........@    ",
      "@..........@   ",
      "@...........@  ",
      "@............@ ",
      "@......@@@@@@@@",
      "@......@       ",
      "@....@@.@      ",
      "@...@ @.@      ",
      "@..@   @.@     ",
      "@.@    @.@     ",
      "@@      @.@    ",
      "@       @.@    ",
      "         @.@   ",
      "         @@@   ",
  };
}  // namespace

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, kColorBlack);
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, kColorWhite);
      } else {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, kMouseTransparentColor);
      }
    }
  }
}