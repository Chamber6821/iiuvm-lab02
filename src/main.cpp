#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
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
    std::cout << timestamp() << ": "
              << (acLine ? "charger is connected" : "charger is disconnected")
              << std::endl;
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

const char *batteryStatus(BYTE batteryFlag) {
  switch (batteryFlag) {
  case 0: return "Eeeeeew, something between low and high";
  case 128: return "Absent";
  case 8 | 1: return "High and charging";
  case 8 | 2: return "Low and charging";
  case 8 | 3: return "Critical and charging";
  case 1: return "High";
  case 2: return "Low";
  case 3: return "Critical";
  default: return "unknown";
  }
}

void boss() {
  const auto column = std::setw(15);
  while (true) {
    auto connection = lastChargerConnection();
    auto disconnection = lastChargerDisconnection();
    auto status = powerStatus();
    std::system("cls");
    std::cout //
      << column << "Status: " << batteryStatus(status.BatteryFlag) << "\n"
      << column << "Charge: " << (int)status.BatteryLifePercent << "%\n"
      << column << "Life time: " << "~" << (int)status.BatteryLifeTime
      << " sec\n";
    if (connection > disconnection)
      std::cout //
        << column << "Charges: " << timestamp() - connection << " sec\n";
    else
      std::cout //
        << column << "Discharges: " << timestamp() - disconnection << " sec\n";
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
