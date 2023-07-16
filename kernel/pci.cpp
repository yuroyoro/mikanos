/**
 * @file pci.cpp
 *
 * PCI バス制御のプログラムを集めたファイル．
 */

#include "pci.hpp"

#include "asmfunc.h"

namespace {
  using namespace pci;

  /** @brief CONFIG_ADDRESS 用の 32 ビット整数を生成する */
  uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr) {
    auto shl = [](uint32_t x, unsigned int bits) {
      return x << bits;
    };

    return shl(1, 31)  // enable bit
           | shl(bus, 16) | shl(device, 11) | shl(function, 8) | (reg_addr & 0xfcu);
  }

  /** @brief devices[num_device] に情報を書き込み num_device をインクリメントする． */
  Error AddDevice(const Device& device) {
    if (num_device == devices.size()) {
      return MAKE_ERROR(Error::kFull);
    }

    devices[num_device] = device;
    ++num_device;
    return MAKE_ERROR(Error::kSuccess);
  }

  Error ScanBus(uint8_t bus);

  /** @brief 指定のファンクションを devices に追加する．
	 * もし PCI-PCI ブリッジなら，セカンダリバスに対し ScanBus を実行する
	 */
  Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
    auto class_code = ReadClassCode(bus, device, function);
    auto header_type = ReadHeaderType(bus, device, function);

    Device dev{bus, device, function, header_type, class_code};

    // add device
    if (auto err = AddDevice(dev)) {
      return err;
    }

    if (class_code.Match(0x06u, 0x04u)) {
      // standard PCI-PCI bridge
      auto bus_numbers = ReadBusNumbers(bus, device, function);
      uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
      return ScanBus(secondary_bus);
    }

    return MAKE_ERROR(Error::kSuccess);
  }

  /** @brief 指定のデバイス番号の各ファンクションをスキャンする．
	 * 有効なファンクションを見つけたら ScanFunction を実行する．
	 */
  Error ScanDevice(uint8_t bus, uint8_t device) {
    // scan function-0
    if (auto err = ScanFunction(bus, device, 0)) {
      return err;
    }

    if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
      return MAKE_ERROR(Error::kSuccess);
    }

    // scan funciton 1-8
    for (uint8_t function = 1; function < 8; ++function) {
      if (IsInvalidVendorId(bus, device, function)) {  // no device
        continue;
      }

      if (auto err = ScanFunction(bus, device, function)) {
        return err;
      }
    }

    return MAKE_ERROR(Error::kSuccess);
  }

  /** @brief 指定のバス番号の各デバイスをスキャンする．
	 * 有効なデバイスを見つけたら ScanDevice を実行する．
	 */
  Error ScanBus(uint8_t bus) {
    // read device 0-31
    for (uint8_t device = 0; device < 32; ++device) {
      if (IsInvalidVendorId(bus, device, 0))  // no device
      {
        continue;
      }

      if (auto err = ScanDevice(bus, device)) {
        return err;
      }
    }

    return MAKE_ERROR(Error::kSuccess);
  }

  /** @brief 指定された MSI ケーパビリティ構造を読み取る
	 *
	 * @param dev  MSI ケーパビリティを読み込む PCI デバイス
	 * @param cap_addr  MSI ケーパビリティレジスタのコンフィグレーション空間アドレス
	 */
  MSICapability ReadMSICapability(const Device& dev, uint8_t cap_addr) {
    MSICapability msi_cap{};

    msi_cap.header.data = ReadConfReg(dev, cap_addr);
    msi_cap.msg_addr = ReadConfReg(dev, cap_addr + 4);

    uint8_t msg_data_addr = cap_addr + 8;
    if (msi_cap.header.bits.addr_64_capable) {
      msi_cap.msg_upper_addr = ReadConfReg(dev, cap_addr + 8);
      msg_data_addr = cap_addr + 12;
    }

    msi_cap.msg_data = ReadConfReg(dev, msg_data_addr);

    if (msi_cap.header.bits.per_vector_mask_capable) {
      msi_cap.mask_bits = ReadConfReg(dev, msg_data_addr + 4);
      msi_cap.pending_bits = ReadConfReg(dev, msg_data_addr + 8);
    }

    return msi_cap;
  }

  /** @brief 指定された MSI ケーパビリティ構造に書き込む
	 *
	 * @param dev  MSI ケーパビリティを読み込む PCI デバイス
	 * @param cap_addr  MSI ケーパビリティレジスタのコンフィグレーション空間アドレス
	 * @param msi_cap  書き込む値
	 */
  void WriteMSICapability(const Device& dev, uint8_t cap_addr, const MSICapability& msi_cap) {
    WriteConfReg(dev, cap_addr, msi_cap.header.data);
    WriteConfReg(dev, cap_addr + 4, msi_cap.msg_addr);

    uint8_t msg_data_addr = cap_addr + 8;
    if (msi_cap.header.bits.addr_64_capable) {
      WriteConfReg(dev, cap_addr + 8, msi_cap.msg_upper_addr);
      msg_data_addr = cap_addr + 12;
    }

    WriteConfReg(dev, msg_data_addr, msi_cap.msg_data);

    if (msi_cap.header.bits.per_vector_mask_capable) {
      WriteConfReg(dev, msg_data_addr + 4, msi_cap.mask_bits);
      WriteConfReg(dev, msg_data_addr + 8, msi_cap.pending_bits);
    }
  }

  /** @brief 指定された MSI レジスタを設定する */
  Error ConfigureMSIRegister(const Device& dev, uint8_t cap_addr, uint32_t msg_addr, uint32_t msg_data, unsigned int num_vector_exponent) {
    auto msi_cap = ReadMSICapability(dev, cap_addr);

    if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent) {
      msi_cap.header.bits.multi_msg_enable =
          msi_cap.header.bits.multi_msg_capable;
    } else {
      msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
    }

    msi_cap.header.bits.msi_enable = 1;
    msi_cap.msg_addr = msg_addr;
    msi_cap.msg_data = msg_data;

    WriteMSICapability(dev, cap_addr, msi_cap);

    return MAKE_ERROR(Error::kSuccess);
  }

  /** @brief 指定された MSI レジスタを設定する */
  Error ConfigureMSIXRegister(const Device& dev, uint8_t cap_addr, uint32_t msg_addr, uint32_t msg_data, unsigned int num_vector_exponent) {
    return MAKE_ERROR(Error::kNotImplemented);
  }

}  // namespace

namespace pci {
  void WriteAddress(uint32_t address) {
    IoOut32(kConfigAddress, address);
  }

  void WriteData(uint32_t value) {
    IoOut32(kConfigData, value);
  }

  uint32_t ReadData() {
    return IoIn32(kConfigData);
  }

  uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() & 0xffffu;
  }

  uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x00));
    return ReadData() >> 16;
  }

  uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x0c));
    return (ReadData() >> 16) & 0xffu;
  }

  ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x08));
    auto reg = ReadData();

    ClassCode cc;
    cc.base = (reg >> 24) & 0xffu;
    cc.sub = (reg >> 16) & 0xffu;
    cc.interface = (reg >> 8) & 0xffu;

    return cc;
  }

  uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
    WriteAddress(MakeAddress(bus, device, function, 0x18));
    return ReadData();
  }

  bool IsSingleFunctionDevice(uint8_t header_type) {
    return (header_type & 0x80u) == 0;
  }

  bool IsInvalidVendorId(uint8_t bus, uint8_t device, uint8_t function) {
    return (ReadVendorId(bus, device, function) == 0xffffu);
  }

  Error ScanAllBus() {
    num_device = 0;

    auto header_type = ReadHeaderType(0, 0, 0);  // Host Bridge
    if (IsSingleFunctionDevice(header_type)) {
      return ScanBus(0);  // Bus-0 Host Bridge
    }

    // have multiple host bridges
    for (uint8_t function = 1; function < 8; ++function) {
      if (IsInvalidVendorId(0, 0, function)) {  // no device on bus
        continue;
      }

      //  function-number means bus-number
      if (auto err = ScanBus(function)) {
        return err;
      }
    }

    return MAKE_ERROR(Error::kSuccess);
  }

  uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr) {
    WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
    return ReadData();
  }

  void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value) {
    WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
    WriteData(value);
  }

  WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index) {
    if (bar_index >= 6) {
      return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
    }

    const auto addr = CalcBarAddress(bar_index);
    const auto bar = ReadConfReg(device, addr);

    // 32 bit address
    if ((bar & 4u) == 0) {
      return {bar, MAKE_ERROR(Error::kSuccess)};
    }

    // 64 bit address
    if (bar_index >= 5) {
      return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
    }

    const auto bar_upper = ReadConfReg(device, addr + 4);
    return {
        bar | (static_cast<uint64_t>(bar_upper) << 32),
        MAKE_ERROR(Error::kSuccess)};
  }

  CapabilityHeader ReadCapabilityHeader(const Device& dev, uint8_t addr) {
    CapabilityHeader header;
    header.data = pci::ReadConfReg(dev, addr);
    return header;
  }

  Error ConfigureMSI(const Device& dev, uint32_t msg_addr, uint32_t msg_data, unsigned int num_vector_exponent) {
    // read capability addrress
    uint8_t cap_addr = ReadConfReg(dev, 0x34) & 0xffu;
    // search MSI/MSIX capabilty addrress
    uint8_t msi_cap_addr = 0, msix_cap_addr = 0;
    while (cap_addr != 0) {
      auto header = ReadCapabilityHeader(dev, cap_addr);
      if (header.bits.cap_id == kCapabilityMSI) {
        msi_cap_addr = cap_addr;
      } else if (header.bits.cap_id == kCapabilityMSIX) {
        msix_cap_addr = cap_addr;
      }
      cap_addr = header.bits.next_ptr;
    }

    if (msi_cap_addr) {
      return ConfigureMSIRegister(dev, msi_cap_addr, msg_addr, msg_data, num_vector_exponent);
    } else if (msix_cap_addr) {
      return ConfigureMSIXRegister(dev, msix_cap_addr, msg_addr, msg_data, num_vector_exponent);
    }

    return MAKE_ERROR(Error::kNoPCIMSI);
  }

  Error ConfigureMSIFixedDestination(
      const Device& dev,
      uint8_t apic_id,
      MSITriggerMode trigger_mode,
      MSIDeliveryMode delivery_mode,
      uint8_t vector,
      unsigned int num_vector_exponent) {
    uint32_t msg_addr = 0xfee00000u | (apic_id << 12);
    uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;
    if (trigger_mode == MSITriggerMode::kLevel) {
      msg_data |= 0xc000;
    }

    return ConfigureMSI(dev, msg_addr, msg_data, num_vector_exponent);
  }
}  // namespace pci