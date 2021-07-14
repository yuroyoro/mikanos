#pragma once

#include "frame_buffer_config.hpp"

template <typename T>
struct Vector2D
{
	T x, y;
};

struct PixelColor
{
	uint8_t r, g, b;
};

const PixelColor kColorBlack{0, 0, 0};
const PixelColor kColorWhite{255, 255, 255};

class PixelWriter
{
public:
	PixelWriter(const FrameBufferConfig &config) : config_{config} {}
	virtual ~PixelWriter() = default;
	virtual void Write(int x, int y, const PixelColor &c) = 0;
	void DrawRectangle(const Vector2D<int> &pos, const Vector2D<int> &size, const PixelColor &c);
	void FillRectangle(const Vector2D<int> &pos, const Vector2D<int> &size, const PixelColor &c);

protected:
	uint8_t *PixelAt(int x, int y)
	{
		return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
	}

private:
	const FrameBufferConfig &config_;
};

class RGBResv8BitPerColorPixleWriter : public PixelWriter
{
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor &c) override;
};

class BGRResv8BitPerColorPixleWriter : public PixelWriter
{
public:
	using PixelWriter::PixelWriter;

	virtual void Write(int x, int y, const PixelColor &c) override;
};
