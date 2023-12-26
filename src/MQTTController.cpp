#include "MQTTController.h"
#include "coop_settings.h"
#include <WiFiManager.h>
#include <vector>
#include <thread>
#include "utils.h"

#include "CoopLogger.h"
#include "coop_time.h"


using std::vector;
using std::thread;

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE


void MQTTController::registerSubscriptions() {
    client.subscribe("coop/#");
}

void MQTTController::publishData() {
//    client.publish("coop/uptime", coop_time::get(coop_time::UPTIME).c_str(), true, 0);
//    client.publish("coop/time", coop_time::get(coop_time::CURRENT_TIME).c_str(), true, 0);
//    client.publish("coop/free_memory", coop_time::get(coop_time::FREE_MEMORY).c_str(), true, 0);
//
//    client.publish("coop/wifi/ssid", coop_wifi::get(coop_wifi::SSID).c_str(), true, 0);
//    client.publish("coop/wifi/ip_address", coop_wifi::get(coop_wifi::IP_ADDRESS).c_str(), true, 0);
//    client.publish("coop/wifi/gateway", coop_wifi::get(coop_wifi::GATEWAY).c_str(), true, 0);
//    client.publish("coop/wifi/subnet", coop_wifi::get(coop_wifi::SUBNET).c_str(), true, 0);
//    client.publish("coop/wifi/dns", coop_wifi::get(coop_wifi::DNS).c_str(), true, 0);
//    client.publish("coop/wifi/mac_address", coop_wifi::get(coop_wifi::MAC_ADDRESS).c_str(), true, 0);
//    client.publish("coop/wifi/rssi", coop_wifi::get(coop_wifi::RSSI).c_str(), true, 0);

    std::scoped_lock l(hasDataItemsMutex);
    for(auto const &item : hasDataItems) {
        const auto &data = item->getData();
        for(auto const &data_item : data) {
            client.publish(("coop/" + item->getInstanceID() + "/" + data_item.first).c_str(), data_item.second.c_str(), true, 0);
        }
    }
}

void MQTTController::messageReceived(String &topic, String &payload) {
    CoopLogger::logi(TAG, "incoming: %s - %s", topic.c_str(), payload.c_str());

    // Note: Do not use the client in the callback to publish, subscribe or
    // unsubscribe as it may cause deadlocks when other things arrive while
    // sending and receiving acknowledgments. Instead, change a global variable,
    // or push to a queue and handle it in the loop after calling `client.loop()`.

    std::scoped_lock l(hasDataItemsMutex);
    if(hasDataItems.empty())
        return;
    const std::vector<std::string> topic_parts = utils::split(topic.c_str(), '/');
    const auto &item_key = topic_parts[topic_parts.size() - 2];
    const auto &item_data_key = topic_parts[topic_parts.size() - 1];
    for(auto const &item : hasDataItems) {
        if(item_key == item->getInstanceID()) {
            item->set(item_data_key, payload.c_str());
            return;
        }
    }
}

void MQTTController::connect() {
    if (WiFi.status() != WL_CONNECTED) 
        CoopLogger::logi(TAG, "No WiFi, will attempt to connect once wifi is established...");
    while (WiFi.status() != WL_CONNECTED && 
        utils::wait_for<std::chrono::seconds>(std::chrono::seconds(1), loopThreadMutex, loopThreadCond, loopThreadStop)) {
        CoopLogger::getDefaultPrintStream()->print("-w-mqtt-");
    }

    if(!lastWillTopic.empty() && !offlineMsg.empty()) 
        client.setWill(lastWillTopic.c_str(), offlineMsg.c_str(), true, 0);

    const char *user = getDataRef()[MQTT_USER].c_str();
    const char *password = getDataRef()[MQTT_PASSWORD].c_str();
    strcpy(clientID, std::string(HOSTNAME "-" + std::to_string(controllerID)).c_str());
    CoopLogger::logi(TAG, "Wifi connected, %s connecting to MQTT as user %s...", clientID, user);
    while (!client.connect(clientID, user, password) &&
        utils::wait_for<std::chrono::seconds>(std::chrono::seconds(10), loopThreadMutex, loopThreadCond, loopThreadStop)) {
        CoopLogger::getDefaultPrintStream()->print("-mqtt-");
    }
    CoopLogger::logi(TAG, "MQTT connected!");
    if(!lastWillTopic.empty() && !onlineMsg.empty()) 
        client.publish(lastWillTopic.c_str(), onlineMsg.c_str(), true, 0);
  
    registerSubscriptions();
}

void MQTTController::disconnect() {
    if(!lastWillTopic.empty() && !offlineMsg.empty()) 
        client.publish(lastWillTopic.c_str(), offlineMsg.c_str(), true, 0);
    client.disconnect();    
    CoopLogger::logi(TAG, "MQTT disconnected!");
}

void MQTTController::mqttLoop() {
    connect();
    while(utils::wait_for<std::chrono::seconds>(std::chrono::seconds(MQTT_PUB_INTERVAL_SECS), loopThreadMutex, loopThreadCond, loopThreadStop)) {
        if (!client.connected()) 
            connect();
        client.loop();
        publishData();
    }
    disconnect();
}

/// @brief Sets the MQTT last will and testament topic and message.
///          This will cause the connection to be re-established.
///          Only one topic/payload pair can be set.
/// @param  lastWillTopic   The last will and testament topic
/// @param  offlineMsg The last will and testament payload and disconnect message
/// @param  onlineMsg  Message send when going online
void MQTTController::setLastWill(const std::string &_lastWillTopic, const std::string &_offlineMsg, const std::string &_onlineMsg) {
    this->lastWillTopic = _lastWillTopic;
    this->offlineMsg = _offlineMsg;
    this->onlineMsg = _onlineMsg;
    if(client.connected()) {
        disconnect();
        connect();
    }
}

void MQTTController::clearLastWill() {
    lastWillTopic.clear();
    offlineMsg.clear();
    onlineMsg.clear();
    if(client.connected()) {
        disconnect();
        connect();
    }
}

void MQTTController::init() {
    CoopLogger::logv(TAG, "[init] MQTT (%d) init", controllerID);
}

void MQTTController::startLoop() {
    std::scoped_lock l(initMutex);

    updateDataVarsFromWifiParams();
    if(!HasData::loadNvsData())
        CoopLogger::loge(TAG, "Failed to load data from NVS");

    std::string &server_str = getDataRef()[MQTT_SERVER];
    if (server_str.empty()) {
        CoopLogger::loge(TAG, "MQTT (%d) server not set", controllerID);
        return;
    }

    const char *server = server_str.c_str();
    const char *port = getDataRef()[MQTT_PORT].c_str();
    const char *user = getDataRef()[MQTT_USER].c_str();
    const char *password = getDataRef()[MQTT_PASSWORD].c_str();

    CoopLogger::logv(TAG, "MQTT (%d) server: %s", controllerID, server_str.c_str());
    CoopLogger::logv(TAG, "MQTT (%d) port: %s", controllerID, port);
    CoopLogger::logv(TAG, "MQTT (%d) user: %s", controllerID, user);
    CoopLogger::logv(TAG, "MQTT (%d) password: %s", controllerID, password);


    IPAddress ip;
    if (ip.fromString(server)) {
        net = new WiFiClient();
        client.begin(ip, atoi(port), *net);
    } else if (strcmp(mqtt_port.getValue(), "1883") ==
               0) { // assume insecure if standard port, otherwise will require addl handling
        net = new WiFiClient();
        client.begin(server, atoi(port), *net);
    } else {
        net = new WiFiClientSecure();
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        ((WiFiClientSecure *) net)->setCACertBundle(x509_crt_imported_bundle_bin_start);
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE   
        client.begin(server, atoi(port), *net);
    }

    client.onMessage([this](String &topic, String &payload) { messageReceived(topic, payload); });

    loopThreadStop = false;
    loopThread = new thread([this](){ mqttLoop(); });
    CoopLogger::logi(TAG, "MQTT (%d) started", controllerID);
}

void MQTTController::stopLoop() {
    std::scoped_lock l(initMutex);
    if(loopThread) {
        {
            std::scoped_lock<std::mutex> _l(loopThreadMutex);
            loopThreadStop = true;
        }
        loopThreadCond.notify_one();
        loopThread->join();
        delete loopThread;
        loopThread = nullptr;
    }
    if(net) {
        delete net;
        net = nullptr;
    }
    CoopLogger::logi(TAG, "MQTT (%d) stopped", controllerID);
}

MQTTController::~MQTTController() {
    stopLoop();
}

void MQTTController::updateDataVarsFromWifiParams() {
    CoopLogger::logv(TAG, "[updateDataVarsFromWifiParams]");
    for(auto &setting : getSettingParamsNoUpdate()) {
        HasData::set(setting->getID(), setting->getValue());
    }
}

void MQTTController::updateWiFiParamsFromDataVars() {
    CoopLogger::logv(TAG, "[updateWiFiParamsFromDataVars]");
    auto l = HasData::getDataLock();
    std::map<std::string, std::string> &_data = HasData::getDataRef();

    for(auto &setting : getSettingParamsNoUpdate()) {
        std::string &value = _data[setting->getID()];
        if(!value.empty())
            setting->setValue(value.c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
    }
}

vector<WiFiManagerParameter *> MQTTController::getSettingParamsNoUpdate() {
    CoopLogger::logv(TAG, "[getSettingParamsNoUpdate]");
    return {&mqtt_server, &mqtt_port, &mqtt_user, &mqtt_password};
}

vector<WiFiManagerParameter *> MQTTController::getSettingParams() {
    CoopLogger::logv(TAG, "[getSettingParams]");
    updateWiFiParamsFromDataVars();
    return getSettingParamsNoUpdate();
}

void MQTTController::afterConfigPageSave() {
    CoopLogger::logv(TAG, "[afterConfigPageSave]");
    updateDataVarsFromWifiParams();
    if(!HasData::saveNvsData())
        CoopLogger::loge(TAG, "Failed to save data to NVS");
}

void MQTTController::registerHasDataItem(HasData *item) {
    std::scoped_lock l(hasDataItemsMutex);
    hasDataItems.emplace_back(item);
}

void MQTTController::unregisterHasDataItem(HasData *item) {
    std::scoped_lock l(hasDataItemsMutex);
    hasDataItems.erase(std::remove(hasDataItems.begin(), hasDataItems.end(), item), hasDataItems.end());
}
