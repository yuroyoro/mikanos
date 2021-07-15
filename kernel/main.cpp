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
#include "pci.hpp"

void operator delete(void *obj) noexcept
{
}

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor = kColorWhite;
const PixelColor kDesktopTaskBarButtonBGColor = {1, 8, 17};
const PixelColor kDesktopTaskBarButtonFGColor = {160, 160, 160};
const PixelColor kDesktopTaskBarColor = {80, 80, 80};

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shpae[kMouseCursorHeight][kMouseCursorWidth + 1] = {
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

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixleWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

int printk(const char *format, ...)
{
	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	result = vsprintf(s, format, ap);
	va_end(ap);

	console->PutString(s);
	return result;
}

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

	const int kFrameWidth = frame_buffer_config.horizontal_resolution;
	const int kFrameHeight = frame_buffer_config.vertical_resolution;

	// draw desktop
	pixel_writer->FillRectangle(
		{0, 0},
		{kFrameWidth, kFrameHeight - 50},
		kDesktopBGColor);

	pixel_writer->FillRectangle(
		{0, kFrameHeight - 50},
		{kFrameWidth, 50},
		kDesktopTaskBarColor);

	pixel_writer->FillRectangle(
		{0, kFrameHeight - 50},
		{kFrameWidth / 5, 50},
		kDesktopTaskBarButtonBGColor);

	pixel_writer->DrawRectangle(
		{10, kFrameHeight - 40},
		{30, 30},
		kDesktopTaskBarButtonFGColor);

	console = new (console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
	printk("Welcom to MikanOS!\n");

	// draw mouse cursor
	for (int dy = 0; dy < kMouseCursorHeight; ++dy)
	{
		for (int dx = 0; dx < kMouseCursorWidth; ++dx)
		{
			if (mouse_cursor_shpae[dy][dx] == '@')
			{
				pixel_writer->Write(200 + dx, 100 + dy, kColorBlack);
			}
			else if (mouse_cursor_shpae[dy][dx] == '.')
			{
				pixel_writer->Write(200 + dx, 100 + dy, kColorWhite);
			}
		}
	}

	// scan PCI devices
	auto err = pci::ScanAllBus();
	printk("ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i)
	{
		const auto &dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
		printk("%d.%d.%d: vend %04x, class %08x, head %02x\n",
			   dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
	}

	while (1)
	{
		__asm__("hlt");
	}
}

extern "C" void __cxa_pure_virtual()
{
	while (1)
		__asm__("hlt");
}
