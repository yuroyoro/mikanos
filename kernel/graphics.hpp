#pragma once

#include "frame_buffer_config.hpp"

struct PixelColor
{
	uint8_t r, g, b;
};

class PixelWriter
{
public:
	PixelWriter(const FrameBufferConfig &config) : config_{config} {}
	virtual ~PixelWriter() = default;
	virtual void Write(int x, int y, const PixelColor &c) = 0;

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
