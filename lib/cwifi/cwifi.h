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

  // https://github.com/tzapu/WiFiManager/blob/master/examples/Super/OnDemandConfigPortal/OnDemandConfigPortal.ino#L147
  inline static WiFiManagerParameter otherWifiConfigParams[]{};
  // https://learn.microsoft.com/en-us/cpp/cpp/string-and-character-literals-cpp?view=msvc-170
  inline static constexpr const char* wifiFaviconHTMLHeader = "<link rel='icon' type='image/png' href='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mBx5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' />";

} // namespace cwifi

#endif // CWIFI_H