#pragma once

#include <cstdint>
#include <memory>
#include <windows.h>

class IO {
  std::shared_ptr<void> handle;

public:
  IO();
  std::uint32_t read(std::uint16_t port) const;
  void write(std::uint16_t port, std::uint32_t value) const;
};
