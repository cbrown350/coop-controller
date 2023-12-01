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
  vector<WiFiManagerParameter*> setupParams;

  bool reset_wifi = false;

  // void IRAM_ATTR wifiResetIsr() {
  //   reset_wifi = true;
  // }

  // void setupWifiReset() {
  //   CoopLogger::logi(TAG, "Setting up WiFi reset on pin %d", WIFI_SETUP_FACTORY_RESET_BUTTON_IN_B);
  //   pinMode(WIFI_SETUP_FACTORY_RESET_BUTTON_IN_B, INPUT_PULLUP);
  //   attachInterrupt(digitalPinToInterrupt(WIFI_SETUP_FACTORY_RESET_BUTTON_IN_B), wifiResetIsr, FALLING);
  //   interrupts();
  // }

  void resetWifi() {
    // reset_wifi = true;
    CoopLogger::logi(TAG, "Resetting WiFi settings");
    WiFi.disconnect(false, true);
  }

  data& getData() {
      return wifi_data;
  }

  void WiFi_Connected(WiFiEvent_t event, WiFiEventInfo_t info) {
    CoopLogger::logi(TAG, "Connected to AP successfully!");
    for (auto onConnectedCallback : onConnectedCallbacks)
      onConnectedCallback();
  }

  void Get_IPAddress(WiFiEvent_t event, WiFiEventInfo_t info) {
    CoopLogger::logi(TAG, "WIFI is connected!");  
    printWifiStatus(*printStream);
    // printStream.println("IP address: ");
    // printStream.println(WiFi.localIP());

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

    // server.begin();
    for (auto onIPAddressCallback : onIPAddressCallbacks)
      onIPAddressCallback();
  }

  void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    CoopLogger::logi(TAG, "Disconnected from WIFI access point");
    CoopLogger::logi(TAG, "WiFi lost connection. Reason: %u", info.wifi_sta_disconnected.reason);
    CoopLogger::logi(TAG, "Reconnecting...");
    // WiFi.begin(ssid, pass);
    // server.end();
    for (auto onDisconnectedCallback : onDisconnectedCallbacks)
      onDisconnectedCallback();
  }

  void addSetupParams(const vector<WiFiManagerParameter*> &setupParams) {
    for(auto setupParam : setupParams) 
      coop_wifi::setupParams.push_back(setupParam);
  }

  void setAPPageParams(WiFiManager &wm) {
    for (auto setupParam : setupParams) {
      wm.addParameter(setupParam);
    }
  }

  void autoConnect(void * args) {    
    
    WiFi.mode(WIFI_STA);
    // wm.resetSettings();
    WiFi.onEvent(WiFi_Connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(Get_IPAddress, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(Wifi_disconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    // listNetworks(*printStream);
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

    // while(true) {
    //   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //   if(reset_wifi) {
    //     CoopLogger::logi(TAG, "Resetting WiFi settings");
    //     WiFi.disconnect(false, true);
    //     // ESP.restart();
    //     break;
    //   }
    // }
    vTaskDelete( NULL );
  }

  void init(Print &printStream,
      const vector<function<void()>> &onConnectedCallbacks, 
      const vector<function<void()>> &onIPAddressCallbacks, 
      const vector<function<void()>> &onDisconnectedCallbacks) {

    coop_wifi::printStream = &printStream;
    coop_wifi::onConnectedCallbacks = onConnectedCallbacks;
    coop_wifi::onIPAddressCallbacks = onIPAddressCallbacks;
    coop_wifi::onDisconnectedCallbacks = onDisconnectedCallbacks;

    // setupWifiReset();

	  // thread startAutoConnect(autoConnect);
    // startAutoConnect.detach(); //NOSONAR - won't fix, intended to run once and end
    // used xTaskCreate instead to be able use larger stack
    xTaskCreate(autoConnect,          /* Task function. */
                "autoConnect",        /* String with name of task. */
                10000,            /* Stack size in bytes. */
                NULL,             /* Parameter passed as input of the task */
                1,                /* Priority of the task. */
                NULL);
  }
  void init(
      const vector<function<void()>> &onConnectedCallbacks,
      const vector<function<void()>> &onIPAddressCallbacks,
      const vector<function<void()>> &onDisconnectedCallbacks) {
    init(*CoopLogger::getDefaultPrintStream(), onConnectedCallbacks, onIPAddressCallbacks, onDisconnectedCallbacks);
  }
  
  void listNetworks(Print &printStream) {
    // scan for nearby networks:
    printStream.println("** Scan WiFi Networks **");

    int16_t numSsid = WiFi.scanNetworks();

    // print the list of networks seen:
    printStream.print("Number of available networks:");
    printStream.println(numSsid);

    // print the network number and name for each network found:
    for (int thisNet = 0; thisNet<numSsid; thisNet++) {
      std::string apValues;
      // printStream.print(thisNet);
      // printStream.print(") ");
      // printStream.print(WiFi.SSID(thisNet));
      // printStream.print("\tSignal: ");
      // printStream.print(WiFi.RSSI(thisNet));
      // printStream.print(" dBm");
      // printStream.print("\tEncryption: ");
      // printStream.print(WiFi.encryptionType(thisNet));
      // printStream.print("\tChannel: ");
      // printStream.println(WiFi.channel(thisNet));
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

      printStream.println(apValues.c_str());
    }
  }
  

  void printWifiStatus(Print &printStream) {
  
    // printStream.print("SSID: ");
    // printStream.println(WiFi.SSID());

    // printStream.print("Channel: ");
    // printStream.println(WiFi.channel());

    // IPAddress ip = WiFi.localIP();
    // printStream.print("IP Address: ");
    // printStream.println(ip);

    // long rssi = WiFi.RSSI();
    // printStream.print("signal strength (RSSI):");
    // printStream.print(rssi);
    // printStream.println(" dBm");
    std::string wifiStatus;
    wifiStatus = "SSID: " + std::string(WiFi.SSID().c_str());
    wifiStatus += "\nChannel: " + std::to_string(WiFi.channel());
    wifiStatus += "\nIP Address: " + std::string(WiFi.localIP().toString().c_str());
    wifiStatus += "\nsignal strength (RSSI):" + std::to_string(WiFi.RSSI()) + " dBm";

    printStream.println(wifiStatus.c_str());
  }

}

