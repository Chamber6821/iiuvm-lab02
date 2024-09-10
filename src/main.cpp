#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>

SYSTEM_POWER_STATUS powerStatus() {
  SYSTEM_POWER_STATUS status{};
  if (not GetSystemPowerStatus(&status))
    throw std::runtime_error("Failed to get power status");
  return status;
}

auto timestamp() {
  return std::chrono::duration_cast<std::chrono::seconds>(
           std::chrono::system_clock::now().time_since_epoch()
  )
    .count();
}

void warden() {
  BYTE oldAcLine = powerStatus().ACLineStatus;
  if (oldAcLine == 255) throw std::runtime_error("AC line has unknown status");
  while (true) {
    Sleep(1000);
    auto acLine = powerStatus().ACLineStatus;
    if (acLine == oldAcLine) continue;
    std::ofstream(acLine ? "charger-connected" : "charger-disconnected")
      << timestamp();
    oldAcLine = acLine;
  }
}

std::int64_t lastChargerConnection() {
  std::int64_t time = 0;
  std::ifstream("charger-connected") >> time;
  return time;
}

std::int64_t lastChargerDisconnection() {
  std::int64_t time = 0;
  std::ifstream("charger-disconnected") >> time;
  return time;
}

void boss() {
  while (true) {
    auto connection = lastChargerConnection();
    auto disconnection = lastChargerDisconnection();
    auto status = powerStatus();
    std::cout //
      << "AC line:              " << (int)status.ACLineStatus << std::endl
      << "Battery flag:         " << (int)status.BatteryFlag << std::endl
      << "Battery life percent: " << (int)status.BatteryLifePercent << std::endl
      << "Battery life time:    " << (int)status.BatteryLifeTime << std::endl
      << "Full life time:       " << (int)status.BatteryFullLifeTime
      << std::endl;
    if (connection > disconnection)
      std::cout //
        << "Works with carger:    " << timestamp() - disconnection << " sec\n";
    else
      std::cout //
        << "Works without carger: " << timestamp() - connection << " sec\n";
    Sleep(1000);
  }
}

int main(int argc, char **argv) {
  std::vector<std::string> args(argv, argv + argc);
  if (args.size() != 2)
    throw std::runtime_error(
      "Mode not specified. Expected one of arguments: warden boss"
    );
  auto mode = args[1];
  if (mode == "warden") warden();
  else if (mode == "boss")
    boss();
}
