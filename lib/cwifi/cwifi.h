#ifndef CWIFI_H
#define CWIFI_H

#include <WebServer.h>
#include <WiFiManager.h>

#include <Logger.h>
#include <HasData.h>
#include <utils.h>

#include <vector>
#include <functional>

namespace cwifi {
    using Logger = Logger<>;

    inline static constexpr const char * const TAG{"cwifi"};

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

  inline static const std::vector<std::string> nvsDataKeys = {};
  inline static const std::vector<std::string> readOnlyKeys{SSID, IP_ADDRESS, GATEWAY, SUBNET, DNS, MAC_ADDRESS, RSSI};
  inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, nvsDataKeys, true);

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
  void listNetworks(Print &_printStream=*Logger::getDefaultPrintStream());
  void printWifiStatus(Print &_printStream=*Logger::getDefaultPrintStream());

  void resetWifi();

  [[nodiscard]] HasData<> &getData();
  [[nodiscard]] std::string get(const std::string &key);

} // namespace cwifi

#endif // CWIFI_H