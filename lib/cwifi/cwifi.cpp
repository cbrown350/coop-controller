#include "settings.h"

#include <WiFiManager.h>
#include <Print.h>

#include <Logger.h>
#include "cwifi.h"
#include "wifi_reason_lookup.h"

#include <thread>
#include <vector>

namespace cwifi {
  using std::thread;
  using std::function;
  using std::vector;

  namespace {
      class _ : public HasData<> {
          public:
              _() : HasData("wifi") {};
              using HasData::getData;
              using HasData::get;

              [[nodiscard]] const char * getTag() const override { return TAG; }

              [[nodiscard]] std::vector<std::string> getNvsKeys() const override { return cwifi::nvsDataKeys; }
              [[nodiscard]] std::vector<std::string> getKeys() const override { return cwifi::readOnlyKeys; }
              [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return cwifi::readOnlyKeys; }

          private:
              using HasData::getNvsNamespace;
              using HasData::loadNvsData;
              using HasData::saveNvsData;
              using HasData::deleteNvsData;
              using HasData::setData;
              using HasData::set;

              [[nodiscard]] std::string getWithOptLock(const std::string &key, bool noLock) const override {
//                  std::unique_lock l(_dataMutex, std::defer_lock);
//
//                  switch(utils::hashstr(key.c_str())) {
//                      case utils::hashstr(SSID): {
//                          if(!noLock)
//                              l.lock();
//                          return WiFi.SSID().c_str();
//                      }
//                      case utils::hashstr(IP_ADDRESS): {
//                          if(!noLock)
//                              l.lock();
//                          return WiFi.localIP().toString().c_str();
//                      }
//                      case utils::hashstr(GATEWAY): {
//                          if(!noLock)
//                              l.lock();
//                          return WiFi.gatewayIP().toString().c_str();
//                      }
//                      case utils::hashstr(SUBNET): {
//                          if(!noLock)
//                              l.lock();
//                          return WiFi.subnetMask().toString().c_str();
//                      }
//                      case utils::hashstr(DNS): {
//                          if(!noLock)
//                              l.lock();
//                          return WiFi.dnsIP().toString().c_str();
//                      }
//                      case utils::hashstr(MAC_ADDRESS): {
//                          if(!noLock)
//                              l.lock();
//                          return WiFi.macAddress().c_str();
//                      }
//                      case utils::hashstr(RSSI): {
//                          if(!noLock)
//                              l.lock();
//                          return std::to_string(WiFi.RSSI());
//                      }
//                      default: {}
//                  }
//                  Logger::logw(TAG, "Invalid key %s", key.c_str());
//                  return HasData::EMPTY_VALUE;

//                  return getStringDataHelper({{SSID, WiFi.SSID()},
//                                              {IP_ADDRESS, WiFi.localIP().toString()},
//                                              {GATEWAY, WiFi.gatewayIP().toString()},
//                                              {SUBNET, WiFi.subnetMask().toString()},
//                                              {DNS, WiFi.dnsIP().toString()},
//                                              {MAC_ADDRESS, WiFi.macAddress()},
//                                              {RSSI, String(WiFi.RSSI())}
//                                             },
//                                             key, noLock);

                  std::unique_lock l(_dataMutex, std::defer_lock);
                  if(!noLock)
                    l.lock();
                  return getStringDataHelper<String>({{SSID, WiFi.SSID()},
                                              {IP_ADDRESS, WiFi.localIP().toString()},
                                              {GATEWAY, WiFi.gatewayIP().toString()},
                                              {SUBNET, WiFi.subnetMask().toString()},
                                              {DNS, WiFi.dnsIP().toString()},
                                              {MAC_ADDRESS, WiFi.macAddress()},
                                              {RSSI, String(WiFi.RSSI())}
                                             },
                                             key, noLock || l.owns_lock(), String{HasData::EMPTY_VALUE}).c_str();
              }


              [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, const bool noLock, const bool doObjUpdate) override {
                  if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
                     std::find(keys.begin(), keys.end(), key) == keys.end()) {
                      Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
                      return false;
                  }

                  bool updated = false;
//                  static std::vector<std::string> keysToUpdateOnObj = {};

//                  std::unique_lock l{_dataMutex, std::defer_lock};
//                  switch(utils::hashstr(key.c_str())) {
//                      case utils::hashstr(SSID): {
//                          if(!noLock)
//                              l.lock();
//                          updated = new_ssid != value;
//                          if(updated){
    //                          new_ssid = value;
    //                          if(updateObj)
    //                              keysToUpdateOnObj.push_back(key);
    //                          }
//                          break;
//                      }
//                      case utils::hashstr(SSID_PASSWORD): {
//                          if(!noLock)
//                              l.lock();
//                          updated = new_ssidPassword != value;
//                          if(updated){
//                          new_ssidPassword = value;
                  //                          if(updateObj)
                  //                              keysToUpdateOnObj.push_back(key);
                  //                          }
//                          break;
//                      }
//                      default: {
        //                    Logger::logw(TAG, "Invalid key %s", key.c_str());
        //                    return false;
        //                }
//                  }
//                  if(doObjUpdate && !keysToUpdateOnObj.empty()) {
//                      const bool objUpdated = updateObj(keysToUpdateOnObj, noLock || l.owns_lock());
//                      if(objUpdated) {
//                          Logger::logv(TAG, "Updated %s", utils::join(keysToUpdateOnObj, ", ").c_str());
//                          keysToUpdateOnObj.clear();
//                      }
//                      return objUpdated;
//                  }
                  return updated;
              }

//              bool updateObj(const std::vector<std::string> &keys, const bool _dataAlreadyLocked) override {
//                  // member vars already set in setWithOptLockAndUpdate(), use them to set timezone and server
//                  bool success = resetWiFiSSID();
//                  if(success) // don't save if new time values don't work
//                      success &= HasData::updateObj(keys, _dataAlreadyLocked);
//                  return success;
//              }
      } data;
  }

  void startAutoConnect();

  HasData<> &getData() {
        return data;
  }
  std::string get(const std::string &key) {
        return data.get(key);
  }

  static Print *printStream = Logger::getDefaultPrintStream();

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
    Logger::logi(TAG, "Resetting WiFi settings");
    WiFi.disconnect(false, true);
  }

  void WiFi_Connected(WiFiEvent_t, WiFiEventInfo_t) {
    Logger::logi(TAG, "[WiFi_Connected] Connected to AP successfully!");
    for (const auto& onConnectedCallback : onConnectedCallbacks)
      onConnectedCallback();
  }

  void Get_IPAddress(WiFiEvent_t, WiFiEventInfo_t) {
    Logger::logi(TAG, "[Get_IPAddress] WIFI is connected!");
    printWifiStatus(printStream != nullptr ? *printStream : Serial);

    for (const auto &onIPAddressCallback : onIPAddressCallbacks)
      onIPAddressCallback();
  }

  void Wifi_disconnected(WiFiEvent_t, WiFiEventInfo_t info) {
    static int disconnectedCount = 0;
    Logger::logi(TAG, "[Wifi_disconnected] Disconnected from WIFI access point");
    Logger::logi(TAG, "WiFi lost connection. Reason: %s", wifi_reason_lookup(info.wifi_sta_disconnected.reason));
    for (const auto &onDisconnectedCallback : onDisconnectedCallbacks)
      onDisconnectedCallback();

    if (disconnectedCount++ > 10) {
        disconnectedCount = 0;
        startAutoConnect();
    }
  }

  void addConfigWithWiFi(ConfigWithWiFi *configWithWiFi) {
    Logger::logv(TAG, "[addConfigWithWiFi] Adding WiFi config");
    cwifi::wiFiConfigs.push_back(configWithWiFi);
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
    Logger::logv(TAG, "[addOnIPAddressCallback] Adding onIPAddressCallback");
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
                Logger::logv(TAG, "[setWifiConfigParams] Adding param: %s", param->getID());
                wm.addParameter(param);
            }
        }
    }
  }

  void saveWifiConfigs() {
      Logger::logv(TAG, "[saveWifiConfigs]");
      cleanupWiFiConfigObjs();
      for (auto &wiFiConfig : wiFiConfigs) {
          wiFiConfig->afterConfigPageSave();
          for(auto &param : wiFiConfig->getSettingParams())
              Logger::logv(TAG, "[saveWifiConfigs] Param %s: %s", param->getID(), param->getValue());
      }
  }

  void autoConnect(void *) {
    Logger::logv(TAG, "[autoConnect]");
    WiFiGenericClass::mode(WIFI_STA);
    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(Get_IPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(Wifi_disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFiManager wm(printStream != nullptr ? *printStream : Serial);

#if defined(ENABLE_LOGGING) && defined(LOG_LEVEL) && LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    wm.setDebugOutput(true);
#else
    wm.setDebugOutput(false);
#endif

    // set custom html head content , inside <head>
    wm.setCustomHeadElement(wifiFaviconHTMLHeader);

//    wm.setTimeout(WIFI_SETUP_TIMEOUT_SECS); // superseded by below functions
    wm.setConnectTimeout(WIFI_SETUP_TIMEOUT_SECS);
    wm.setConfigPortalTimeout(WIFI_SETUP_TIMEOUT_SECS);
    // wm.setAPCallback(configModeCallback); // called on AP portal start -> configModeCallback (WiFiManager *myWiFiManager)
    wm.setSaveConfigCallback(saveWifiConfigs);
    wm.setConfigPortalTimeoutCallback(saveWifiConfigs);
    setWifiConfigParams(wm);
    while(!wm.autoConnect(CONTROLLER_AP_NAME, CONTROLLER_AP_PASSWORD)) {
        Logger::loge(TAG, "Failed to connect");
//        ESP.restart();
    }

    vTaskDelete( nullptr );
  }

  void init(Print &_printStream,
      const vector<function<void()>> &_onConnectedCallbacks,
      const vector<function<void()>> &_onIPAddressCallbacks,
      const vector<function<void()>> &_onDisconnectedCallbacks) {

      cwifi::printStream = &_printStream;
      for (auto &onConnectedCallback: _onConnectedCallbacks)
          cwifi::onConnectedCallbacks.push_back(onConnectedCallback);
      for (auto &onIPAddressCallback: _onIPAddressCallbacks)
          cwifi::onIPAddressCallbacks.push_back(onIPAddressCallback);
      for (auto &onDisconnectedCallback: _onDisconnectedCallbacks)
          cwifi::onDisconnectedCallbacks.push_back(onDisconnectedCallback);

      Logger::logv(TAG, "[init] Starting WiFi Manager");
      startAutoConnect();
  }

  void init(const vector<function<void()>> &_onConnectedCallbacks,
            const vector<function<void()>> &_onIPAddressCallbacks,
            const vector<function<void()>> &_onDisconnectedCallbacks) {
        init(*Logger::getDefaultPrintStream(), _onConnectedCallbacks,
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
    Logger::logv(TAG, "[startAutoConnect] autoConnectTaskHandle state: %d", state);
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

} // namespace

