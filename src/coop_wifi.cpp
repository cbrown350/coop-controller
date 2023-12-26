#include "coop_settings.h"
#include "CoopLogger.h"
#include "coop_wifi.h"

#include <WiFiManager.h>  

#include <Print.h>
#include "HardwareSerial.h"
#include <thread>
#include <vector>
#include "utils.h"



namespace coop_wifi {
  using std::thread;
  using std::function;
  using std::vector;

  static constexpr const char * const TAG = "cwifi";

  namespace {
      class _ : public HasData {
          public:
              _() : HasData("wifi") {};
              using HasData::getData;
              using HasData::get;

              [[nodiscard]] std::vector<std::string> getKeys() const override { return readOnlyKeys; }
              [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }

          private:
              using HasData::getNvsKeys;
              using HasData::getNvsNamespace;
              using HasData::loadNvsData;
              using HasData::saveNvsData;
              using HasData::deleteNvsData;
              using HasData::setData;

              [[nodiscard]] bool set(const std::string &key, const std::string &value, bool noLock) override { return false; }

              [[nodiscard]] std::string get(const std::string &key, bool noLock) const override {
                  std::unique_lock l(_dataMutex, std::defer_lock);

                  switch(utils::hashstr(key.c_str())) {
                      case utils::hashstr(SSID): {
                          if(!noLock)
                              l.lock();
                          return WiFi.SSID().c_str();
                      }
                      case utils::hashstr(IP_ADDRESS): {
                          if(!noLock)
                              l.lock();
                          return WiFi.localIP().toString().c_str();
                      }
                      case utils::hashstr(GATEWAY): {
                          if(!noLock)
                              l.lock();
                          return WiFi.gatewayIP().toString().c_str();
                      }
                      case utils::hashstr(SUBNET): {
                          if(!noLock)
                              l.lock();
                          return WiFi.subnetMask().toString().c_str();
                      }
                      case utils::hashstr(DNS): {
                          if(!noLock)
                              l.lock();
                          return WiFi.dnsIP().toString().c_str();
                      }
                      case utils::hashstr(MAC_ADDRESS): {
                          if(!noLock)
                              l.lock();
                          return WiFi.macAddress().c_str();
                      }
                      case utils::hashstr(RSSI): {
                          if(!noLock)
                              l.lock();
                          return std::to_string(WiFi.RSSI());
                      }
                      default: {}
                  }
                  CoopLogger::logw(TAG, "Invalid key %s", key.c_str());
                  return HasData::EMPTY_VALUE;
              }
      } data;
  }

  void startAutoConnect();

  HasData &getData() {
        return data;
  }
  std::string get(const std::string &key) {
        return data.get(key);
  }

  static Print *printStream = CoopLogger::getDefaultPrintStream();

  vector<function<void()>> onConnectedCallbacks;
  vector<function<void()>> onIPAddressCallbacks;
  vector<function<void()>> onDisconnectedCallbacks;
  vector<ConfigWithWiFi*> wiFiConfigs;

  void cleanupWiFiConfigObjs() {
      wiFiConfigs.erase(std::remove_if(wiFiConfigs.begin(), wiFiConfigs.end(),
                                       [](ConfigWithWiFi *hasParams) -> bool { return hasParams == nullptr; }),
                        wiFiConfigs.end());
  }

  void resetWifi() {
    CoopLogger::logi(TAG, "Resetting WiFi settings");
    WiFi.disconnect(false, true);
  }

  void WiFi_Connected(WiFiEvent_t, WiFiEventInfo_t) {
    CoopLogger::logi(TAG, "[WiFi_Connected] Connected to AP successfully!");
    for (const auto& onConnectedCallback : onConnectedCallbacks)
      onConnectedCallback();
  }

  void Get_IPAddress(WiFiEvent_t, WiFiEventInfo_t) {
    CoopLogger::logi(TAG, "[Get_IPAddress] WIFI is connected!");
    printWifiStatus(*printStream);

    for (const auto &onIPAddressCallback : onIPAddressCallbacks)
      onIPAddressCallback();
  }

  void Wifi_disconnected(WiFiEvent_t, WiFiEventInfo_t info) {
    static int disconnectedCount = 0;
    CoopLogger::logi(TAG, "[Wifi_disconnected] Disconnected from WIFI access point");
    CoopLogger::logi(TAG, "WiFi lost connection. Reason: %u", info.wifi_sta_disconnected.reason);
    for (const auto &onDisconnectedCallback : onDisconnectedCallbacks)
      onDisconnectedCallback();

    if (disconnectedCount++ > 10) {
        disconnectedCount = 0;
        startAutoConnect();
    }
  }

  void addConfigWithWiFi(ConfigWithWiFi *configWithWiFi) {
    CoopLogger::logv(TAG, "[addConfigWithWiFi] Adding WiFi config");
    coop_wifi::wiFiConfigs.push_back(configWithWiFi);
  }

  const function<void()> * addOnConnectedCallback(const function<void()> &onConnectedCallback) {
    onConnectedCallbacks.push_back(onConnectedCallback);
    return &onConnectedCallbacks.back();
  }

  void removeOnConnectedCallback(const std::function<void()> *onConnectedCallback) {
    onConnectedCallbacks.erase(std::remove_if(onConnectedCallbacks.begin(), onConnectedCallbacks.end(),
                                               [onConnectedCallback](const std::function<void()> &callback) -> bool {
                                                   return &callback == onConnectedCallback;
                                               }),
                                  onConnectedCallbacks.end());
  }

  const function<void()> * addOnIPAddressCallback(const function<void()> &onIPAddressCallback) {
    CoopLogger::logv(TAG, "[addOnIPAddressCallback] Adding onIPAddressCallback");
    onIPAddressCallbacks.push_back(onIPAddressCallback);
    return &onIPAddressCallbacks.back();
  }

  void removeOnIPAddressCallback(const std::function<void()> *onIPAddressCallback) {
    onIPAddressCallbacks.erase(std::remove_if(onIPAddressCallbacks.begin(), onIPAddressCallbacks.end(),
                                               [onIPAddressCallback](const std::function<void()> &callback) -> bool {
                                                   return &callback == onIPAddressCallback;
                                               }),
                                  onIPAddressCallbacks.end());
  }

  const function<void()> * addOnDisconnectedCallback(const function<void()> &onDisconnectedCallback) {
    onDisconnectedCallbacks.push_back(onDisconnectedCallback);
    return &onDisconnectedCallbacks.back();
  }

  void removeOnDisconnectedCallback(const std::function<void()> *onDisconnectedCallback) {
    onDisconnectedCallbacks.erase(std::remove_if(onDisconnectedCallbacks.begin(), onDisconnectedCallbacks.end(),
                                               [onDisconnectedCallback](const std::function<void()> &callback) -> bool {
                                                   return &callback == onDisconnectedCallback;
                                               }),
                                  onDisconnectedCallbacks.end());
  }

  void setWifiConfigParams(WiFiManager &wm) {
    cleanupWiFiConfigObjs();
    for (auto setupParamObj : wiFiConfigs) {
        for (auto param : setupParamObj->getSettingParams()) {
            bool addIt = true;
            for(int i=0; i<wm.getParametersCount(); i++) {
                if(wm.getParameters()[i] == param) {
                    addIt = false;
                    break;
                }
            }
            if(addIt) {
                CoopLogger::logv(TAG, "[setWifiConfigParams] Adding param: %s", param->getID());
                wm.addParameter(param);
            }
        }
    }
  }

  void saveWifiConfigs() {
      CoopLogger::logv(TAG, "[saveWifiConfigs]");
      cleanupWiFiConfigObjs();
      for (auto &wiFiConfig : wiFiConfigs) {
          wiFiConfig->afterConfigPageSave();
          for(auto &param : wiFiConfig->getSettingParams())
              CoopLogger::logv(TAG, "[saveWifiConfigs] Param %s: %s", param->getID(), param->getValue());
      }
  }

  void autoConnect(void *) {
    CoopLogger::logv(TAG, "[autoConnect]");
    WiFiGenericClass::mode(WIFI_STA);
    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(Get_IPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(Wifi_disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFiManager wm(*printStream);

#if defined(ENABLE_LOGGING) && defined(COOP_LOG_LEVEL) && COOP_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    wm.setDebugOutput(true);
#else
    wm.setDebugOutput(false);
#endif

//    wm.setTimeout(WIFI_SETUP_TIMEOUT_SECS); // superseded by below functions
    wm.setConnectTimeout(WIFI_SETUP_TIMEOUT_SECS);
    wm.setConfigPortalTimeout(WIFI_SETUP_TIMEOUT_SECS);
    // wm.setAPCallback(configModeCallback); // called on AP portal start -> configModeCallback (WiFiManager *myWiFiManager)
    wm.setSaveConfigCallback(saveWifiConfigs);
    wm.setConfigPortalTimeoutCallback(saveWifiConfigs);
    setWifiConfigParams(wm);
    while(!wm.autoConnect(COOP_CONTROLLER_AP_NAME, COOP_CONTROLLER_AP_PASSWORD)) {
        CoopLogger::loge(TAG, "Failed to connect");
//        ESP.restart();
    }

    vTaskDelete( nullptr );
  }

  void init(Print &_printStream,
      const vector<function<void()>> &_onConnectedCallbacks,
      const vector<function<void()>> &_onIPAddressCallbacks,
      const vector<function<void()>> &_onDisconnectedCallbacks) {

      coop_wifi::printStream = &_printStream;
      for (auto &onConnectedCallback: _onConnectedCallbacks)
          coop_wifi::onConnectedCallbacks.push_back(onConnectedCallback);
      for (auto &onIPAddressCallback: _onIPAddressCallbacks)
          coop_wifi::onIPAddressCallbacks.push_back(onIPAddressCallback);
      for (auto &onDisconnectedCallback: _onDisconnectedCallbacks)
          coop_wifi::onDisconnectedCallbacks.push_back(onDisconnectedCallback);

      CoopLogger::logv(TAG, "[init] Starting WiFi Manager");
      startAutoConnect();
  }

  void init(const vector<function<void()>> &_onConnectedCallbacks,
            const vector<function<void()>> &_onIPAddressCallbacks,
            const vector<function<void()>> &_onDisconnectedCallbacks) {
        init(*CoopLogger::getDefaultPrintStream(), _onConnectedCallbacks,
             _onIPAddressCallbacks, _onDisconnectedCallbacks);
  }

  void startAutoConnect() {
	// thread startAutoConnect(autoConnect);
    // startAutoConnect.detach(); //NOSONAR - won't fix, intended to run once and end
    // used xTaskCreate instead to be able to use larger stack
    TaskHandle_t xHandle = xTaskGetHandle( "autoConnect" );
    eTaskState state = eDeleted;
    if(xHandle != nullptr)
        state = eTaskGetState(xHandle);
    CoopLogger::logv(TAG, "[startAutoConnect] autoConnectTaskHandle state: %d", state);
    if(state == eDeleted)
        xTaskCreate(autoConnect,          /* Task function. */
                        "autoConnect",        /* String with name of task. */
                        10000,            /* Stack size in bytes. */
                        nullptr,             /* Parameter passed as input of the task */
                        1,                /* Priority of the task. */
                    nullptr);
  }
  
  void listNetworks(Print &_printStream) {
    // scan for nearby networks:
    _printStream.println("** Scan WiFi Networks **");

    int16_t numSsid = WiFi.scanNetworks();

    // print the list of networks seen:
    _printStream.print("Number of available networks:");
    _printStream.println(numSsid);

    // print the network number and name for each network found:
    for (int thisNet = 0; thisNet<numSsid; thisNet++) {
      std::string apValues;
      apValues += std::to_string(thisNet);
      apValues += ") ";
      apValues += WiFi.SSID(thisNet).c_str();
      apValues += "\t\tSignal: ";
      apValues += std::to_string(WiFi.RSSI(thisNet));
      apValues += " dBm";
      apValues += "\tEncryption: ";
      apValues += std::to_string(WiFi.encryptionType(thisNet));
      apValues += "\tChannel: ";
      apValues += std::to_string(WiFi.channel(thisNet));

      _printStream.println(apValues.c_str());
    }
  }
  

  void printWifiStatus(Print &_printStream) {
    std::string wifiStatus;
    wifiStatus = "SSID: " + std::string(WiFi.SSID().c_str());
    wifiStatus += "\nChannel: " + std::to_string(WiFi.channel());
    wifiStatus += "\nIP Address: " + std::string(WiFi.localIP().toString().c_str());
    wifiStatus += "\nsignal strength (RSSI):" + std::to_string(WiFi.RSSI()) + " dBm";

    _printStream.println(wifiStatus.c_str());
  }

}

