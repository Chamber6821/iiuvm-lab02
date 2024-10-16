#include "IO.hpp"
#include <algorithm>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PciConfig {

constexpr int AddressPort = 0x0CF8;
constexpr int DataPort = 0x0CFC;

class Address {

public:
  std::uint8_t offset;
  std::uint8_t function : 3;
  std::uint8_t slot : 5;
  std::uint8_t bus;

private:
  std::uint8_t reserved : 7 = 0;
  std::uint8_t enable : 1 = 1;

public:
  Address(
    std::uint8_t bus, std::uint8_t slot, std::uint8_t function,
    std::uint8_t offset
  )
      : bus(bus), slot(slot), function(function), offset(offset) {}

  std::uint32_t asBytes() const {
    return reinterpret_cast<const std::uint32_t &>(*this);
  }
};

struct Register0 {
  std::uint16_t vendorId;
  std::uint16_t deviceId;

  Register0(std::uint16_t vendorId, std::uint16_t deviceId)
      : vendorId(vendorId), deviceId(deviceId) {}

  Register0(std::uint32_t bytes)
      : Register0(bytes & 0xFFFF, (bytes >> 16) & 0xFFFF) {}

  bool available() const { return vendorId != 0xFFFF; }

  auto operator<=>(const Register0 &other) const = default;
};

struct Device {
  Address address;
  Register0 register0;

  Device(Address address, Register0 register0)
      : address(std::move(address)), register0(std::move(register0)) {}

  Device(IO io, Address address) : address(address), register0(0xFFFF, 0xFFFF) {
    io.write(AddressPort, address.asBytes());
    register0 = io.read(DataPort);
  }
};

} // namespace PciConfig

struct DeviceInfo {
  std::string vendorName;
  std::string deviceName;
};

template <class T>
T parseHex(std::string_view str) {
  T buffer;
  std::stringstream ss;
  ss << std::hex << str;
  ss >> buffer;
  return buffer;
}

template <class T>
std::string toHex(T number) {
  return std::format("{:0{}x}", number, sizeof(number) * 2);
}

auto vendorsMap(std::istream &&in) {
  auto vendorNames = std::map<std::uint16_t, std::string>();
  auto deviceNames =
    std::map<std::uint16_t, std::map<std::uint16_t, std::string>>();
  std::string line;
  std::getline(in, line);
  while (not in.fail()) {
    if (line.empty() or line.starts_with("#")) {
      std::getline(in, line);
      continue;
    }
    auto vendorId = parseHex<std::uint16_t>(line.substr(0, 4));
    auto vendorName = line.substr(6);
    vendorNames[vendorId] = vendorName;

    while (not std::getline(in, line).fail()) {
      if (line.empty() or line.starts_with("#")) continue;
      if (line[0] != '\t') break;
      if (line[1] == '\t') continue;
      auto deviceId = parseHex<std::uint16_t>(line.substr(1, 4));
      auto deviceName = line.substr(7);
      deviceNames[vendorId][deviceId] = deviceName;
    }
  }
  return [vendorNames = std::move(vendorNames),
          deviceNames = std::move(deviceNames)](const PciConfig::Register0 &reg
         ) mutable -> DeviceInfo {
    auto vendor = vendorNames[reg.vendorId];
    auto device = deviceNames[reg.vendorId][reg.deviceId];
    return {
      .vendorName = vendor.empty() ? "N/A" : std::move(vendor),
      .deviceName = device.empty() ? "N/A" : std::move(device),
    };
  };
}

std::vector<PciConfig::Device> allDevices() {
  std::vector<PciConfig::Device> devices;
  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      for (int func = 0; func < 8; func++) {
        auto device =
          PciConfig::Device(IO(), PciConfig::Address(bus, slot, func, 0));
        if (not device.register0.available()) continue;
        devices.emplace_back(std::move(device));
      }
    }
  }
  return devices;
}

int main() {
  std::cout << "Parse pci.ids...\n";
  auto infoMap = vendorsMap(std::ifstream("pci.ids"));
  std::cout << "Scan all devices...\n";
  auto devices = allDevices();
  std::cout << "Sort devices...\n";
  std::ranges::sort(devices, {}, &PciConfig::Device::register0);
  std::cout << "Print devices:\n";
  constexpr std::string_view tableRowFormat =
    "|{:4}|{:4}|{:4}|{:4}|{:4}|{:45}|{:75}\n";
  std::cout << std::format(
    tableRowFormat,
    "BUS",
    "SLOT",
    "FUNC",
    "VID",
    "DID",
    "Vendor Name",
    "Device Name"
  );
  for (auto &device : devices) {
    auto address = device.address;
    auto register0 = device.register0;
    auto info = infoMap(register0);
    std::cout << std::format(
      tableRowFormat,
      address.bus,
      int(address.slot),
      int(address.function),
      toHex(register0.vendorId),
      toHex(register0.deviceId),
      info.vendorName,
      info.deviceName
    );
  }
}
