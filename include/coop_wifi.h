#ifndef COOP_WIFI_H
#define COOP_WIFI_H

#include <vector>
#include <functional>
#include "HardwareSerial.h"

#include "coop_settings.h"
#include <WiFiManager.h>
#include "CoopLogger.h"
#include "HasData.h"


namespace coop_wifi {
  /*
   * Data keys:
   *   ssid
   *   ip_address
   *   gateway
   *   subnet
   *   dns
   *   mac_address
   *   rssi
   *
   *   get values, ex.: get(SSID);
   */
  inline static constexpr const char * const SSID = "ssid";
  inline static constexpr const char * const IP_ADDRESS = "ip_address";
  inline static constexpr const char * const GATEWAY = "gateway";
  inline static constexpr const char * const SUBNET = "subnet";
  inline static constexpr const char * const DNS = "dns";
  inline static constexpr const char * const MAC_ADDRESS = "mac_address";
  inline static constexpr const char * const RSSI = "rssi";

  inline static const std::vector<std::string> readOnlyKeys{SSID, IP_ADDRESS, GATEWAY, SUBNET, DNS, MAC_ADDRESS, RSSI};

  class ConfigWithWiFi {
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

//  void addWiFiConfig(const std::vector<WiFiManagerParameter*> &wiFiConfigs);
  void addConfigWithWiFi(ConfigWithWiFi * configWithWifi);
  void listNetworks(Print &_printStream=*CoopLogger::getDefaultPrintStream());
  void printWifiStatus(Print &_printStream=*CoopLogger::getDefaultPrintStream());

  void resetWifi();

  [[nodiscard]] HasData &getData();
  [[nodiscard]] std::string get(const std::string &key);

} // namespace coop_wifi

#endif // COOP_WIFI_H