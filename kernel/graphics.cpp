/**
 * @file graphics.cpp
 *
 * 画像描画関連のプログラムを集めたファイル．
 */

#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
  auto p = PixelAt(pos);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c) {
  auto p = PixelAt(pos);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
  // draw horizontal line
  for (int dx = 0; dx < size.x; ++dx) {
    writer.Write(pos + Vector2D<int>{dx, 0}, c);
    writer.Write(pos + Vector2D<int>{dx, size.y - 1}, c);
  }
  // draw vertical line
  for (int dy = 1; dy < size.y - 1; ++dy) {
    writer.Write(pos + Vector2D<int>{0, dy}, c);
    writer.Write(pos + Vector2D<int>{size.x - 1, dy}, c);
  }
}

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      writer.Write(pos + Vector2D<int>{dx, dy}, c);
    }
  }
}

void DrawDesktop(PixelWriter& writer) {
  const auto width = writer.Width();
  const auto height = writer.Height();

  FillRectangle(writer,
                {0, 0},
                {width, height - kDesktopStatusBarHeight},
                kDesktopBGColor);

  FillRectangle(writer,
                {0, height - kDesktopStatusBarHeight},
                {width, kDesktopStatusBarHeight},
                kDesktopStatusBarColor);

  FillRectangle(writer,
                {0, height - kDesktopStatusBarHeight},
                {width / kDesktopStartMenuRatio, kDesktopStatusBarHeight},
                kDesktopStartMenuBGColor);

  DrawRectangle(writer,
                {kDesktopStartMenuMargin, height - (kDesktopStatusBarHeight - kDesktopStartMenuMargin)},
                {kDesktopStartMenuWidth, kDesktopStartMenuHeight},
                kDesktopStartMenuFGColor);
}
