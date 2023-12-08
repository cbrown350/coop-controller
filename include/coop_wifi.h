#ifndef COOP_WIFI_H
#define COOP_WIFI_H

#include <vector>
#include <functional>
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

  class HasConfigPageParams {
    public:
      [[nodiscard]] virtual std::vector<WiFiManagerParameter*> getSettingParams() = 0;
      virtual void afterConfigPageSave() = 0;
  };

  void init(Print &_printStream,
      const std::vector<std::function<void()>> &onConnectedCallbacks={}, 
      const std::vector<std::function<void()>> &onIPAddressCallbacks={}, 
      const std::vector<std::function<void()>> &onDisconnectedCallbacks={});
  void init(
      const std::vector<std::function<void()>> &onConnectedCallbacks={}, 
      const std::vector<std::function<void()>> &onIPAddressCallbacks={}, 
      const std::vector<std::function<void()>> &onDisconnectedCallbacks={});

  const std::function<void()> * addOnConnectedCallback(const std::function<void()> &onConnectedCallback);
  void removeOnConnectedCallback(const std::function<void()> *onConnectedCallback);
  const std::function<void()> * addOnIPAddressCallback(const std::function<void()> &onIPAddressCallback);
  void removeOnIPAddressCallback(const std::function<void()> *onIPAddressCallback);
  const std::function<void()> * addOnDisconnectedCallback(const std::function<void()> &onDisconnectedCallback);
  void removeOnDisconnectedCallback(const std::function<void()> *onDisconnectedCallback);

//  void addSetupParams(const std::vector<WiFiManagerParameter*> &setupParamObjs);
  void addSetupParams(HasConfigPageParams *hasParams);
  void listNetworks(Print &_printStream=*CoopLogger::getDefaultPrintStream());
  void printWifiStatus(Print &_printStream=*CoopLogger::getDefaultPrintStream());

  void resetWifi();

  data& getData();

  } // namespace coop_wifi

#endif // COOP_WIFI_H