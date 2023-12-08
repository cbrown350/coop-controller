#include "coop_settings.h"
#include "CoopLogger.h"
#include "coop_wifi.h"

#include <Arduino.h> 
#include <WiFiManager.h>  

#include <Print.h>
#include "HardwareSerial.h"
#include <thread>
#include <vector>


namespace coop_wifi {
  using std::thread;
  using std::function;
  using std::vector;

  static constexpr const char * const TAG = "cwifi";

  data wifi_data;

  Print *printStream = &Serial;

  vector<function<void()>> onConnectedCallbacks;
  vector<function<void()>> onIPAddressCallbacks;
  vector<function<void()>> onDisconnectedCallbacks;
  vector<HasConfigPageParams*> setupParamObjs;

  void cleanupSetupParamObjs() {
      setupParamObjs.erase(std::remove_if(setupParamObjs.begin(), setupParamObjs.end(),
                                          [](HasConfigPageParams *hasParams) -> bool { return hasParams == nullptr; }),
                           setupParamObjs.end());
  }

  void resetWifi() {
    CoopLogger::logi(TAG, "Resetting WiFi settings");
    WiFi.disconnect(false, true);
  }

  data& getData() {
      return wifi_data;
  }

  void WiFi_Connected(WiFiEvent_t event, WiFiEventInfo_t info) {
    CoopLogger::logi(TAG, "Connected to AP successfully!");
    for (const auto& onConnectedCallback : onConnectedCallbacks)
      onConnectedCallback();
  }

  void Get_IPAddress(WiFiEvent_t event, WiFiEventInfo_t info) {
    CoopLogger::logi(TAG, "WIFI is connected!");  
    printWifiStatus(*printStream);

    // esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    // esp_netif_sntp_init(&config);
    // esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    // esp_sntp_setservername(0, "pool.ntp.org");
    // esp_sntp_init();

      // sntp_setoperatingmode(SNTP_OPMODE_POLL);
      // sntp_setservername(1, "pool.ntp.org");
      // ip_addr_t ip;
      // ipaddr_aton("192.168.2.233", &ip);
      // sntp_setserver(2, &ip);
      // sntp_init();
      // setenv("TZ","MST7MDT,M3.2.0,M11.1.0",1); 
      // tzset();

    configTime(-3600 * 6, 0, "pool.ntp.org", "192.168.2.233");

    cleanupSetupParamObjs();
    for (auto setupParamObj : setupParamObjs)
        setupParamObj->afterConfigPageSave();

    for (const auto &onIPAddressCallback : onIPAddressCallbacks)
      onIPAddressCallback();
  }

  void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    CoopLogger::logi(TAG, "Disconnected from WIFI access point");
    CoopLogger::logi(TAG, "WiFi lost connection. Reason: %u", info.wifi_sta_disconnected.reason);
    for (const auto &onDisconnectedCallback : onDisconnectedCallbacks)
      onDisconnectedCallback();
  }

//  void addSetupParams(const vector<WiFiManagerParameter*> &setupParamObjs) {
//    for(auto setupParam : setupParamObjs)
//      coop_wifi::setupParamObjs.push_back(setupParam);
//  }

  void addSetupParams(HasConfigPageParams *hasParams) {
//    addSetupParams(hasParams->getSettingParams());
    coop_wifi::setupParamObjs.push_back(hasParams);
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

  void setAPPageParams(WiFiManager &wm) {
      cleanupSetupParamObjs();
    for (auto setupParamObj : setupParamObjs) {
        for (auto param : setupParamObj->getSettingParams()) {
            wm.addParameter(param);
        }
    }
  }

  void autoConnect(void * args) {    
    
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(Get_IPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(Wifi_disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFiManager wm(*printStream);
    wm.setTimeout(WIFI_SETUP_TIMEOUT_SECS);
    // wm.setAPCallback(configModeCallback); // called on AP portal start -> configModeCallback (WiFiManager *myWiFiManager)
    setAPPageParams(wm);
    if(!wm.autoConnect(COOP_CONTROLLER_AP_NAME, COOP_CONTROLLER_AP_PASSWORD)) {
        CoopLogger::loge(TAG, "Failed to connect");
        ESP.restart();
    // } else {
    //     //if you get here you have connected to the WiFi    
    //     printStream.println("connected...yeey :) starting web server");
    //     // server.begin();
    //     for (auto onConnectedCallback : onConnectedCallbacks)
    //       onConnectedCallback();
    }

    vTaskDelete( nullptr );
  }

  void init(Print &_printStream,
      const vector<function<void()>> &_onConnectedCallbacks,
      const vector<function<void()>> &_onIPAddressCallbacks,
      const vector<function<void()>> &_onDisconnectedCallbacks) {

    coop_wifi::printStream = &_printStream;
    coop_wifi::onConnectedCallbacks = _onConnectedCallbacks;
    coop_wifi::onIPAddressCallbacks = _onIPAddressCallbacks;
    coop_wifi::onDisconnectedCallbacks = _onDisconnectedCallbacks;

	  // thread startAutoConnect(autoConnect);
    // startAutoConnect.detach(); //NOSONAR - won't fix, intended to run once and end
    // used xTaskCreate instead to be able use larger stack
    xTaskCreate(autoConnect,          /* Task function. */
                "autoConnect",        /* String with name of task. */
                10000,            /* Stack size in bytes. */
                nullptr,             /* Parameter passed as input of the task */
                1,                /* Priority of the task. */
                nullptr);
  }
  void init(
      const vector<function<void()>> &_onConnectedCallbacks,
      const vector<function<void()>> &_onIPAddressCallbacks,
      const vector<function<void()>> &_onDisconnectedCallbacks) {
    init(*CoopLogger::getDefaultPrintStream(), _onConnectedCallbacks,
         _onIPAddressCallbacks, _onDisconnectedCallbacks);
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

