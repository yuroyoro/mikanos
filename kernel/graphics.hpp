#pragma once

#include "frame_buffer_config.hpp"

template <typename T>
struct Vector2D {
  T x, y;

  template <typename U>
  Vector2D<T>& operator+=(const Vector2D<U>& rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
};

struct PixelColor {
  uint8_t r, g, b;
};

inline bool operator==(const PixelColor& lhs, const PixelColor& rhs) {
  return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}

inline bool operator!=(const PixelColor& lhs, const PixelColor& rhs) {
  return !(lhs == rhs);
}

const PixelColor kColorBlack{0, 0, 0};
const PixelColor kColorWhite{255, 255, 255};

class PixelWriter {
public:
  virtual ~PixelWriter() = default;
  virtual void Write(int x, int y, const PixelColor& c) = 0;
  virtual int Width() const = 0;
  virtual int Height() const = 0;

  void DrawRectangle(const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c);
  void FillRectangle(const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c);
};

class FrameBufferWriter : public PixelWriter {
public:
  FrameBufferWriter(const FrameBufferConfig& config) : config_{config} {
  }
  virtual ~FrameBufferWriter() = default;
  virtual int Width() const override { return config_.horizontal_resolution; }
  virtual int Height() const override { return config_.vertical_resolution; }

protected:
  uint8_t* PixelAt(int x, int y) {
    return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
  }

private:
  const FrameBufferConfig& config_;
};

class RGBResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
  using FrameBufferWriter::FrameBufferWriter;

  virtual void Write(int x, int y, const PixelColor& c) override;
};

class BGRResv8BitPerColorPixelWriter : public FrameBufferWriter {
public:
  using FrameBufferWriter::FrameBufferWriter;

  virtual void Write(int x, int y, const PixelColor& c) override;
};

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor = kColorWhite;
const PixelColor kDesktopStartMenuBGColor = {80, 80, 80};
const PixelColor kDesktopStartMenuFGColor = {160, 160, 160};
const PixelColor kDesktopStatusBarColor = {1, 8, 17};

const int kDesktopStatusBarHeight = 50;
const int kDesktopStartMenuRatio = 5;
const int kDesktopStartMenuMargin = 10;
const int kDesktopStartMenuWidth = 30;
const int kDesktopStartMenuHeight = 30;

void DrawDesktop(PixelWriter& writer);
