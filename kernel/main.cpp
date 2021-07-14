/**
 * @file main.cpp
 *
 * カーネル本体のプログラムを書いたファイル．
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "console.hpp"

void *operator new(size_t size, void *buf)
{
	return buf;
}

void operator delete(void *obj) noexcept
{
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixleWriter)];
PixelWriter *pixel_writer;

extern "C" void KernelMain(
	const FrameBufferConfig &frame_buffer_config)
{
	// initialize PixelWriter
	switch (frame_buffer_config.pixel_format)
	{
	case kPixelRGBResv8BitPerColor:
		pixel_writer = new (pixel_writer_buf)
			RGBResv8BitPerColorPixleWriter{frame_buffer_config};
		break;
	case kPixelBGRResv8BitPerColor:
		pixel_writer = new (pixel_writer_buf)
			BGRResv8BitPerColorPixleWriter{frame_buffer_config};
		break;
	}

	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x)
	{
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y)
		{
			pixel_writer->Write(x, y, {255, 255, 255});
		}
	}

	Console console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

	char buf[128];
	for (int i = 0; i < 27; ++i)
	{
		sprintf(buf, "line %d\n", i);
		console.PutString(buf);
	}

	while (1)
	{
		__asm__("hlt");
	}
}