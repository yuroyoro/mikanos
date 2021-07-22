/**
 * @file graphics.cpp
 *
 * 画像描画関連のプログラムを集めたファイル．
 */

#include "graphics.hpp"

void PixelWriter::DrawRectangle(const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
  // draw horizontal line
  for (int dx = 0; dx < size.x; ++dx) {
    Write(pos.x + dx, pos.y, c);
    Write(pos.x + dx, pos.y + size.y - 1, c);
  }
  // draw vertical line
  for (int dy = 0; dy < size.y; ++dy) {
    Write(pos.x, pos.y + dy, c);
    Write(pos.x + size.x - 1, pos.y + dy, c);
  }
}

void PixelWriter::FillRectangle(const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
  for (int dy = 0; dy < size.y; ++dy) {
    for (int dx = 0; dx < size.x; ++dx) {
      Write(pos.x + dx, pos.y + dy, c);
    }
  }
}

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
  auto p = PixelAt(x, y);
  p[0] = c.r;
  p[1] = c.g;
  p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
  auto p = PixelAt(x, y);
  p[0] = c.b;
  p[1] = c.g;
  p[2] = c.r;
}

void DrawDesktop(PixelWriter& writer) {
  const auto width = writer.Width();
  const auto height = writer.Height();

  writer.FillRectangle(
      {0, 0},
      {width, height - kDesktopStatusBarHeight},
      kDesktopBGColor);

  writer.FillRectangle(
      {0, height - kDesktopStatusBarHeight},
      {width, kDesktopStatusBarHeight},
      kDesktopStatusBarColor);

  writer.FillRectangle(
      {0, height - kDesktopStatusBarHeight},
      {width / kDesktopStartMenuRatio, kDesktopStatusBarHeight},
      kDesktopStartMenuBGColor);

  writer.DrawRectangle(
      {kDesktopStartMenuMargin, height - (kDesktopStatusBarHeight - kDesktopStartMenuMargin)},
      {kDesktopStartMenuWidth, kDesktopStartMenuHeight},
      kDesktopStartMenuFGColor);
}
