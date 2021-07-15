/**
 * @file pci.cpp
 *
 * PCI バス制御のプログラムを集めたファイル．
 */

#include "pci.hpp"
#include "asmfunc.h"

namespace
{
	using namespace pci;

	/** CONFIG_ADDRESS 用の 32 ビット整数を生成する */
	uint32_t MakeAddress(uint8_t bus, uint8_t device,
						 uint8_t function, uint8_t reg_addr)
	{
		auto shl = [](uint32_t x, unsigned int bits)
		{
			return x << bits;
		};

		return shl(1, 31) // enable bit
			   | shl(bus, 16) | shl(device, 11) | shl(function, 8) | (reg_addr & 0xfcu);
	}

	/**  devices[num_device] に情報を書き込み num_device をインクリメントする． */
	Error AddDevice(uint8_t bus, uint8_t device,
					uint8_t function, uint8_t header_type)
	{

		if (num_device == devices.size())
		{
			return MAKE_ERROR(Error::kFull);
		}

		devices[num_device] = Device{bus, device, function, header_type};
		++num_device;
		return MAKE_ERROR(Error::kSuccess);
	}

	Error ScanBus(uint8_t bus);

	/**  指定のファンクションを devices に追加する．
	 * もし PCI-PCI ブリッジなら，セカンダリバスに対し ScanBus を実行する
	 */
	Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function)
	{
		auto header_type = ReadHeaderType(bus, device, function);

		// add device
		if (auto err = AddDevice(bus, device, function, header_type))
		{
			return err;
		}

		auto class_code = ReadClassCode(bus, device, function);
		uint8_t base = (class_code >> 24) & 0xffu;
		uint8_t sub = (class_code >> 16) & 0xffu;

		if (base == 0x06u && sub == 0x04u)
		{
			// standard PCI-PCI bridge
			auto bus_numbers = ReadBusNumbers(bus, device, function);
			uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
			return ScanBus(secondary_bus);
		}

		return MAKE_ERROR(Error::kSuccess);
	}

	/**  指定のデバイス番号の各ファンクションをスキャンする．
	 * 有効なファンクションを見つけたら ScanFunction を実行する．
	 */
	Error ScanDevice(uint8_t bus, uint8_t device)
	{
		// scan function-0
		if (auto err = ScanFunction(bus, device, 0))
		{
			return err;
		}

		if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0)))
		{
			return MAKE_ERROR(Error::kSuccess);
		}

		// scan funciton 1-8
		for (uint8_t function = 1; function < 8; ++function)
		{
			if (IsInvalidVendorId(bus, device, function))
			{ // no device
				continue;
			}

			if (auto err = ScanFunction(bus, device, function))
			{
				return err;
			}
		}

		return MAKE_ERROR(Error::kSuccess);
	}

	/**  指定のバス番号の各デバイスをスキャンする．
	 * 有効なデバイスを見つけたら ScanDevice を実行する．
	 */
	Error ScanBus(uint8_t bus)
	{
		// read device 0-31
		for (uint8_t device = 0; device < 32; ++device)
		{
			if (IsInvalidVendorId(bus, device, 0)) // no device
			{
				continue;
			}

			if (auto err = ScanDevice(bus, device))
			{
				return err;
			}
		}

		return MAKE_ERROR(Error::kSuccess);
	}
}

namespace pci
{
	void WriteAddress(uint32_t address)
	{
		IoOut32(kConfigAddress, address);
	}

	void WriteData(uint32_t value)
	{
		IoOut32(kConfigData, value);
	}

	uint32_t ReadData()
	{
		return IoIn32(kConfigData);
	}

	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function)
	{
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() & 0xffffu;
	}

	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function)
	{
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() >> 16;
	}

	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function)
	{
		WriteAddress(MakeAddress(bus, device, function, 0x0c));
		return (ReadData() >> 16) & 0xffu;
	}

	uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function)
	{
		WriteAddress(MakeAddress(bus, device, function, 0x08));
		return ReadData();
	}

	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function)
	{
		WriteAddress(MakeAddress(bus, device, function, 0x18));
		return ReadData();
	}

	bool IsSingleFunctionDevice(uint8_t header_type)
	{
		return (header_type & 0x80u) == 0;
	}

	bool IsInvalidVendorId(uint8_t bus, uint8_t device, uint8_t function)
	{
		return (ReadVendorId(bus, device, function) == 0xffffu);
	}

	Error ScanAllBus()
	{
		num_device = 0;

		auto header_type = ReadHeaderType(0, 0, 0); // Host Bridge
		if (IsSingleFunctionDevice(header_type))
		{
			return ScanBus(0); // Bus-0 Host Bridge
		}

		// have multiple host bridges
		for (uint8_t function = 1; function < 8; ++function)
		{
			if (IsInvalidVendorId(0, 0, function)) // no device on bus
			{
				continue;
			}

			//  function-number means bus-number
			if (auto err = ScanBus(function))
			{
				return err;
			}
		}

		return MAKE_ERROR(Error::kSuccess);
	}
}