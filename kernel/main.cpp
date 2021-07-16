/**
 * @file main.cpp
 *
 * カーネル本体のプログラムを書いたファイル．
 */

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "mouse.hpp"
#include "logger.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"

#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor = kColorWhite;
const PixelColor kDesktopStatusBarButtonBGColor = {1, 8, 17};
const PixelColor kDesktopStatusBarButtonFGColor = {160, 160, 160};
const PixelColor kDesktopStatusBarColor = {80, 80, 80};

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixleWriter)];
PixelWriter *pixel_writer;

char console_buf[sizeof(Console)];
Console *console;

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y)
{
	mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

void SwitchEhci2Xhci(const pci::Device &xhc_dev)
{
	bool intel_ehc_exist = false;
	for (int i = 0; i < pci::num_device; ++i)
	{
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
			0x8086 == pci::ReadVendorId(pci::devices[i]))
		{
			intel_ehc_exist = true;
			break;
		}
	}

	if (!intel_ehc_exist)
	{
		return;
	}

	uint32_t superspeeed_ports = pci::ReadConfReg(xhc_dev, 0xdc); // USB3PRM
	pci::WriteConfReg(xhc_dev, 0xd8, superspeeed_ports);		  // USB3_PSSEN
	uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);	  // XUSB2PRM
	pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);			  // USB3_PSSEN
	Log(kDebug, "SwitchEhci2Xhci:SS = %02, xHCI = %02x\n", superspeeed_ports, ehci2xhci_ports);
}

usb::xhci::Controller *xhc;

/* XHCI Interrupt handler */
__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame)
{
	while (xhc->PrimaryEventRing()->HasFront())
	{
		if (auto err = ProcessEvent(*xhc))
		{
			Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
				err.Name(), err.File(), err.Line());
		}
	}

	NotifyEndOfInterrupt();
}

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
		kDesktopStatusBarColor);

	pixel_writer->FillRectangle(
		{0, kFrameHeight - 50},
		{kFrameWidth / 5, 50},
		kDesktopStatusBarButtonBGColor);

	pixel_writer->DrawRectangle(
		{10, kFrameHeight - 40},
		{30, 30},
		kDesktopStatusBarButtonFGColor);

	console = new (console_buf) Console{
		*pixel_writer, kDesktopFGColor, kDesktopBGColor};

	printk("Welcom to MikanOS!\n");
	SetLogLevel(kWarn);

	// draw mouse cursor
	mouse_cursor = new (mouse_cursor_buf) MouseCursor{
		pixel_writer, kDesktopBGColor, {300, 200}};

	// scan PCI devices
	auto err = pci::ScanAllBus();
	printk("ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i)
	{
		const auto &dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
		Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n",
			dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
	}

	// Intel 製を優先して xHC を探す
	pci::Device *xhc_dev = nullptr;
	for (int i = 0; i < pci::num_device; ++i)
	{
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u))
		{
			xhc_dev = &pci::devices[i];

			if (0x8086 == pci::ReadVendorId(*xhc_dev))
			{
				break;
			}
		}
	}

	if (xhc_dev)
	{
		Log(kInfo, "xHC has been found: %d.%d.%d\n",
			xhc_dev->bus, xhc_dev->device, xhc_dev->function);
	}

	// read code segement
	const uint16_t cs = GetCS();
	// set XHCI interupt handler
	SetIDTEntry(
		idt[InterruptVector::kXHCI],
		MakeIDTAttr(DescriptorType::kInterruptGate, 0),
		reinterpret_cast<uint64_t>(IntHandlerXHCI),
		cs);

	// setup IDT
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

	// setup msi interrupt to xhc
	const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
	pci::ConfigureMSIFixedDestination(
		*xhc_dev, bsp_local_apic_id,
		pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed,
		InterruptVector::kXHCI, 0);

	const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
	Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
	const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
	Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

	usb::xhci::Controller xhc{xhc_mmio_base};

	if (0x8086 == pci::ReadVendorId(*xhc_dev))
	{
		SwitchEhci2Xhci(*xhc_dev);
	}
	{
		auto err = xhc.Initialize();
		Log(kDebug, "xhc.Initialize: %s\n", err.Name());
	}
	Log(kInfo, "xHC starting\n");
	xhc.Run();

	::xhc = &xhc;
	__asm__("sti");

	usb::HIDMouseDriver::default_observer = MouseObserver;

	for (int i = 1; i <= xhc.MaxPorts(); i++)
	{
		auto port = xhc.PortAt(i);
		Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

		if (port.IsConnected())
		{
			if (auto err = ConfigurePort(xhc, port))
			{
				Log(kError, "failed to configure port: %s at %s:%d\n",
					err.Name(), err.File(), err.Line());
				continue;
			}
		}
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
