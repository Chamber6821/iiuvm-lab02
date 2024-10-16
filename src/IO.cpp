#include "IO.hpp"
#include <cstdint>
#include <format>
#include <handleapi.h>
#include <iostream>
#include <memory>
#include <stdexcept>

extern "C" {
#include "driver.h"
}

IO::IO() {
  auto handle = CreateFileW(
    L"\\??\\PortIoDriver",
    GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL
  );
  if (handle == INVALID_HANDLE_VALUE) {
    throw std::runtime_error(
      std::format("Open device file error: {}", GetLastError())
    );
  }
  this->handle =
    std::shared_ptr<void>(handle, [](void *handle) { CloseHandle(handle); });
}

std::uint32_t IO::read(std::uint16_t port) const {
  IoRequestRead request;
  request.port = port;
  IoResponse response = {0};
  if (not DeviceIoControl(
        handle.get(),
        READ_CODE,
        &request,
        sizeof(request),
        &response,
        sizeof(response),
        NULL,
        NULL
      )) {
    throw std::runtime_error(std::format("Read error: {}", GetLastError()));
  }
  // std::cout << std::format("---- {:x} data {:x}\n", port, response.value);
  return response.value;
}

void IO::write(std::uint16_t port, std::uint32_t data) const {
  // std::cout << std::format("Port {:x} data {:x}\n", port, data);
  IoRequestWrite request;
  request.port = port;
  request.value = data;
  if (not DeviceIoControl(
        handle.get(),
        WRITE_CODE,
        &request,
        sizeof(request),
        NULL,
        0,
        NULL,
        NULL
      )) {
    throw std::runtime_error(std::format("Write error: {}", GetLastError()));
  }
}
