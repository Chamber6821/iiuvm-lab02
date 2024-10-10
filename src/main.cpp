#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <windows.h>

extern "C" {
#include "driver.h"
}

#define IOCTL_READ_PORT                                                        \
  CTL_CODE(                                                                    \
    FILE_DEVICE_UNKNOWN,                                                       \
    READ_CODE,                                                                 \
    METHOD_BUFFERED,                                                           \
    GENERIC_READ | GENERIC_WRITE                                               \
  )
#define IOCTL_WRITE_PORT                                                       \
  CTL_CODE(                                                                    \
    FILE_DEVICE_UNKNOWN,                                                       \
    WRITE_CODE,                                                                \
    METHOD_BUFFERED,                                                           \
    GENERIC_READ | GENERIC_WRITE                                               \
  )

using PortWrite = IoRequestWrite;
using PortRead = IoRequestRead;
using PortData = IoResponse;

class IO {
public:
  IO() {
    hDevice = CreateFileW(
      L"\\??\\My_PCI",
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL
    );
  }

  void write(uint16_t port, uint32_t value) {
    PortWrite request;
    request.port = port;
    request.value = value;
    DeviceIoControl(
      hDevice,
      IOCTL_WRITE_PORT,
      &request,
      sizeof(request),
      NULL,
      0,
      NULL,
      NULL
    );
  }

  uint32_t read(uint16_t port) {
    PortRead request;
    request.port = port;
    PortData response = {0};
    DeviceIoControl(
      hDevice,
      IOCTL_READ_PORT,
      &request,
      sizeof(request),
      &response,
      sizeof(response),
      NULL,
      NULL
    );
    return response.value;
  }

private:
  HANDLE hDevice;
};

#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA    0x0CFC
#define MAGIC_CONSTANT     0x80000000
#define OFFSET_CUTOFF      0xFC
#define GET_PCI_DEVICE_ADDR(bus, slot, func, offset)                           \
  ((bus << 16) | (slot << 11) | (func << 8) | (offset & OFFSET_CUTOFF) |       \
   MAGIC_CONSTANT)

struct PCI_DEVICE_RECORD {
  uint16_t VENDOR_ID;
  uint16_t DEVICE_ID;
};

struct PCI_DEVICE_INFO {
  int bus;
  int slot;
  int func;
  std::string vid;
  std::string did;
  std::string vname;
  std::string dname;
};

uint32_t readPCIDeviceInfoPart(int bus, int slot, int func, int offset) {
  IO io;
  uint32_t address = GET_PCI_DEVICE_ADDR(bus, slot, func, offset);
  io.write(PCI_CONFIG_ADDRESS, address);
  return io.read(PCI_CONFIG_DATA);
}

PCI_DEVICE_RECORD readPCIDeviceInfo(int bus, int slot, int func) {
  PCI_DEVICE_RECORD record;
  uint32_t *cursor = (uint32_t *)&record;
  for (int i = 0; i < sizeof(PCI_DEVICE_RECORD) / sizeof(uint32_t); i++) {
    cursor[i] = readPCIDeviceInfoPart(bus, slot, func, i * sizeof(uint32_t));
  }
  return record;
}

std::unordered_map<std::string, std::string> vendorMap;
std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
  deviceMap;

void parsePciIds() {
  std::ifstream file("pci.ids");
  if (!file.is_open()) {
    std::cerr << "pci.ids not found" << std::endl;
    exit(-1);
  }

  std::string line;
  std::string currentVendor;

  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    if (line[0] == '\t') {
      std::string deviceId = line.substr(1, 4);
      std::string deviceName = line.substr(7);
      deviceMap[currentVendor][deviceId] = deviceName;
      continue;
    }

    currentVendor = line.substr(0, 4);
    std::string vname = line.substr(6);
    vendorMap[currentVendor] = vname;
  }

  file.close();
}

std::string shortToHex(unsigned short num) {
  char buffer[5] = {0};
  sprintf_s(buffer, 5, "%04x", num);

  return std::string(buffer);
}

std::string findVendorName(unsigned short vendorId) {
  std::string vendorName = vendorMap[shortToHex(vendorId)];

  return (vendorName.empty() ? "N/A" : vendorName);
}

std::string findDeviceName(unsigned short vendorId, unsigned short deviceId) {
  std::string deviceName =
    deviceMap[shortToHex(vendorId)][shortToHex(deviceId)];

  return (deviceName.empty() ? "N/A" : deviceName);
}

void printTableDeviceRecord(PCI_DEVICE_INFO info) {
  std::cout << "|" << std::hex << std::setw(3) << info.bus << "|"
            << std::setw(4) << info.slot << "|" << std::setw(4) << info.func
            << "|" << std::setw(4) << info.vid << "|" << std::setw(4)
            << info.did << "|" << std::setw(40) << info.vname << "|"
            << std::setw(70) << info.dname << "|" << std::dec << std::endl;
}

void printTableHeader() {
  std::cout << "|" << "BUS|" << "SLOT|" << "FUNC|" << "V_ID|" << "D_ID|"
            << std::setw(41) << "V_NM|" << std::setw(71) << "D_NM|"
            << std::endl;
}

bool isDevicePresent(int bus, int slot, int func) {
  return readPCIDeviceInfoPart(bus, slot, func, 0) != 0xffffffff;
}

bool comparator(const PCI_DEVICE_INFO &dev1, const PCI_DEVICE_INFO &dev2) {
  int difference = dev1.vname.compare(dev2.vname);

  if (difference < 0) return true;

  if (difference > 0) return false;

  return dev1.dname.compare(dev2.dname) < 0;
}

int main() {
  parsePciIds();

  std::vector<PCI_DEVICE_INFO> devices;

  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      for (int func = 0; func < 8; func++) {
        if (!isDevicePresent(bus, slot, func)) continue;

        PCI_DEVICE_RECORD record = readPCIDeviceInfo(bus, slot, func);

        devices.push_back(
          {bus,
           slot,
           func,
           shortToHex(record.VENDOR_ID),
           shortToHex(record.DEVICE_ID),
           findVendorName(record.VENDOR_ID),
           findDeviceName(record.VENDOR_ID, record.DEVICE_ID)}
        );
      }
    }
  }

  std::sort(devices.begin(), devices.end(), comparator);
  printTableHeader();
  for (auto &el : devices) {
    printTableDeviceRecord(el);
  }
}
