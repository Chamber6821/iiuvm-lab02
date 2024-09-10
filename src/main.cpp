#include <iostream>
#include <stdexcept>
#include <windows.h>

SYSTEM_POWER_STATUS powerStatus() {
  SYSTEM_POWER_STATUS status{};
  if (not GetSystemPowerStatus(&status))
    throw std::runtime_error("Failed to get power status");
  return status;
}

int main(int argc, char **argv) {

  auto status = powerStatus();
  std::cout //
    << "AC:                   " << (int)status.ACLineStatus << std::endl
    << "Battery flag:         " << (int)status.BatteryFlag << std::endl
    << "Battery life percent: " << (int)status.BatteryLifePercent << std::endl
    << "Battery life time:    " << (int)status.BatteryLifeTime << std::endl
    << "Full life time:       " << (int)status.BatteryFullLifeTime << std::endl;
}
