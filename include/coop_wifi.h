#ifndef COOP_WIFI_H
#define COOP_WIFI_H

#include <vector>
#include "HardwareSerial.h"

#include "coop_settings.h"
#include <WiFiManager.h>
#include "CoopLogger.h"


namespace coop_wifi {

  using data = struct {
    double rssi;
    const char * rssi_unit = "dBm";
    char * ssid;
    char * ip;
  };

  void init(Print &printStream,
      const std::vector<std::function<void()>> &onConnectedCallbacks={}, 
      const std::vector<std::function<void()>> &onIPAddressCallbacks={}, 
      const std::vector<std::function<void()>> &onDisconnectedCallbacks={});
  void init(
      const std::vector<std::function<void()>> &onConnectedCallbacks={}, 
      const std::vector<std::function<void()>> &onIPAddressCallbacks={}, 
      const std::vector<std::function<void()>> &onDisconnectedCallbacks={});
  
  void addSetupParams(const std::vector<WiFiManagerParameter*> &setupParams);
  void listNetworks(Print &printStream=*CoopLogger::getDefaultPrintStream());
  void printWifiStatus(Print &printStream=*CoopLogger::getDefaultPrintStream());

  void resetWifi();

  data& getData();

  } // namespace coop_wifi

#endif // COOP_WIFI_H