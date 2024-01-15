#include "MQTTController.h"
#include "settings.h"

#include <WiFiManager.h>

#include <Logger.h>
#include <utils.h>

#include <vector>
#include <thread>


using std::vector;
using std::thread;

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
//extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE


void MQTTController::registerSubscriptions() {
    client.subscribe((topicPrefix + "/update/#").c_str());
}

void MQTTController::publishData() {
//    client.publish("coop/uptime", ntp_time::get(ntp_time::UPTIME).c_str(), true, 0);
//    client.publish("coop/time", ntp_time::get(ntp_time::CURRENT_TIME).c_str(), true, 0);
//    client.publish("coop/free_memory", ntp_time::get(ntp_time::FREE_MEMORY).c_str(), true, 0);
//
//    client.publish("coop/wifi/ssid", cwifi::get(cwifi::SSID).c_str(), true, 0);
//    client.publish("coop/wifi/ip_address", cwifi::get(cwifi::IP_ADDRESS).c_str(), true, 0);
//    client.publish("coop/wifi/gateway", cwifi::get(cwifi::GATEWAY).c_str(), true, 0);
//    client.publish("coop/wifi/subnet", cwifi::get(cwifi::SUBNET).c_str(), true, 0);
//    client.publish("coop/wifi/dns", cwifi::get(cwifi::DNS).c_str(), true, 0);
//    client.publish("coop/wifi/mac_address", cwifi::get(cwifi::MAC_ADDRESS).c_str(), true, 0);
//    client.publish("coop/wifi/rssi", cwifi::get(cwifi::RSSI).c_str(), true, 0);

    std::scoped_lock l(hasDataItemsMutex);
    for(auto const &item : hasDataItems) {
        const auto &data = item->getData();
        for(auto const &data_item : data) {
            client.publish((topicPrefix + "/status/" + item->getInstanceID() + "/" + data_item.first).c_str(), data_item.second.c_str(), true, 0);
        }
    }
}

void MQTTController::messageReceived(String &topic, String &payload) {
    Logger::logd(TAG, "incoming: %s - %s", topic.c_str(), payload.c_str());

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
        const auto itemReadOnlyKeys = item->getReadOnlyKeys();
        if(item_key == item->getInstanceID() &&
                std::find(itemReadOnlyKeys.begin(), itemReadOnlyKeys.end(), item_data_key) == itemReadOnlyKeys.end()) {
            if(!item->set(item_data_key, payload.c_str())) {
                Logger::logw(TAG, "Failed to set %s to %s, may have not changed or was invalid", item_data_key.c_str(), payload.c_str());
            }
            return;
        }
    }
}

void MQTTController::connect() {
    Logger::logv(TAG, "[connect]");
    if (WiFiSTAClass::status() != WL_CONNECTED)
        Logger::logi(TAG, "No WiFi, will attempt to connect once wifi is established...");
    while (WiFiSTAClass::status() != WL_CONNECTED) {
        if(!utils::wait_for<std::chrono::seconds>(std::chrono::seconds(1), loopThreadMutex, loopThreadCond, loopThreadStop))
            return;
        Logger::getDefaultPrintStream()->print("-w-mqtt-");
    }

    if(!lastWillTopic.empty() && !offlineMsg.empty()) 
        client.setWill(lastWillTopic.c_str(), offlineMsg.c_str(), true, 0);

    strcpy(clientID, std::string(HOSTNAME "-" + std::to_string(controllerID)).c_str());
    Logger::logi(TAG, "Wifi connected, %s connecting to MQTT as user %s...", clientID, mqttUser.c_str());
    while (!client.connect(clientID, mqttUser.c_str(), mqttPassword.c_str())) {
        if(!utils::wait_for<std::chrono::seconds>(std::chrono::seconds(10), loopThreadMutex, loopThreadCond, loopThreadStop))
            return;
        Logger::getDefaultPrintStream()->print("-mqtt-");
    }
    Logger::logi(TAG, "MQTT connected!");
    if(!lastWillTopic.empty() && !onlineMsg.empty()) 
        client.publish(lastWillTopic.c_str(), onlineMsg.c_str(), true, 0);
  
    registerSubscriptions();
}

void MQTTController::disconnect() {
    Logger::logv(TAG, "[disconnect]");
        
    if (client.connected()) {
        if(!lastWillTopic.empty() && !offlineMsg.empty()) 
            client.publish(lastWillTopic.c_str(), offlineMsg.c_str(), true, 0);
        client.disconnect();    
        Logger::logi(TAG, "MQTT disconnected!");
    }
}

void MQTTController::mqttLoop() {
    static unsigned long lastPub = 0;
    connect();
    while(utils::wait_for<std::chrono::milliseconds>(std::chrono::milliseconds(10), loopThreadMutex, loopThreadCond, loopThreadStop)) {
        if (!client.connected())
            connect();
        client.loop();
        if(millis() - lastPub > MQTT_PUB_INTERVAL_SECS * 1000) {
            publishData();
            lastPub = millis();
        }
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
    Logger::logv(TAG, "[init] MQTT (%d) init", controllerID);
    if(!loadNvsData()) 
        Logger::loge(TAG, "[MQTTController] Failed to load data from NVS");
}

void MQTTController::startLoop() {
    Logger::logv(TAG, "[startLoop] MQTT (%d) startLoop", controllerID);

    // if(!loadNvsData())//TODO: don't load on start loop since will be loaded on init
    //     Logger::loge(TAG, "Failed to load data from NVS");

    Logger::logv(TAG, "MQTT (%d) server: %s", controllerID, mqttServer.c_str());
    Logger::logv(TAG, "MQTT (%d) port: %d", controllerID, mqttPort);

    {
        std::scoped_lock<std::mutex> _l(loopThreadMutex);

        IPAddress ip;
        if (ip.fromString(mqttServer.c_str())) {
            net = new WiFiClient();
            client.begin(ip, (int)mqttPort, *net);
        } else if (mqttPort == 1883) { // assume insecure if standard port, otherwise will require addl handling
            net = new WiFiClient();
            client.begin(mqttServer.c_str(), (int)mqttPort, *net);
        } else {
            net = new WiFiClientSecure();
    #ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
            ((WiFiClientSecure *) net)->setCACertBundle(x509_crt_imported_bundle_bin_start);
    #endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
            client.begin(mqttServer.c_str(), (int)mqttPort, *net);
        }

        client.onMessage([this](String &topic, String &payload) { messageReceived(topic, payload); });

        loopThreadStop = false;
        loopThread = new thread([this]() { mqttLoop(); });
    }
    Logger::logi(TAG, "MQTT (%d) started", controllerID);
}

void MQTTController::stopLoop() {
    Logger::logv(TAG, "[stopLoop] MQTT (%d) stopLoop", controllerID);
    if(loopThread) {
        {
            std::scoped_lock<std::mutex> _l(loopThreadMutex);
            loopThreadStop = true;
        }
        loopThreadCond.notify_all();
        loopThread->join();
        delete loopThread;
        loopThread = nullptr;
    }
    if(net) {
        delete net;
        net = nullptr;
    }
    Logger::logi(TAG, "MQTT (%d) stopped", controllerID);
}

// if running, stops loop and then starts loop
bool MQTTController::restart() {
    if(!loopThreadStop) {
        Logger::logi(TAG, "Restarting MQTT (%d)...", controllerID);
        stopLoop();
        startLoop();
        return true;
    }
    Logger::logi(TAG, "MQTT (%d) not running, not restarting", controllerID);
    return false;
}

MQTTController::~MQTTController() {
    stopLoop();
}

void MQTTController::updateDataVarsFromWifiParams() {
    Logger::logv(TAG, "[updateDataVarsFromWifiParams]");
    std::map<std::string, std::string> dataToUpdate = {};
    for(WiFiManagerParameter *setting : getSettingParamsNoUpdate()) {
        // find key from setting ID by checking ID starts with key
        const std::string id = setting->getID();
        const auto &key= std::find_if(keys.begin(), keys.end(),
                                      [&id](const std::string &key) { return id.find(key) == 0; });
        if(key != keys.end())
            dataToUpdate[*key] = setting->getValue();
    }
    if(!setData(dataToUpdate))
        Logger::loge(TAG, "Failed to save data from WiFi params");
}

void MQTTController::updateWiFiParamsFromDataVars() {
    Logger::logv(TAG, "[updateWiFiParamsFromDataVars]");
    for(auto &setting : getSettingParamsNoUpdate()) {
        // find key from setting ID by checking ID starts with key
        const std::string id = setting->getID();
        const auto &key= std::find_if(keys.begin(), keys.end(),
                                      [&id](const std::string &key) { return id.find(key) == 0; });
        if(key != keys.end()) {
            const std::string value = get(*key);
//            if (!value.empty())
                setting->setValue(value.c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
        }
    }
}

vector<WiFiManagerParameter *> MQTTController::getSettingParamsNoUpdate() {
    Logger::logv(TAG, "[getSettingParamsNoUpdate]");
    return {&mqtt_server_param, &mqtt_port_param, &mqtt_user_param, &mqtt_password_param};
}

vector<WiFiManagerParameter *> MQTTController::getSettingParams() {
    Logger::logv(TAG, "[getSettingParams]");
    updateWiFiParamsFromDataVars();
    return getSettingParamsNoUpdate();
}

void MQTTController::afterConfigPageSave() {
    Logger::logv(TAG, "[afterConfigPageSave]");
    updateDataVarsFromWifiParams();
}

void MQTTController::registerHasDataItem(HasData *item) {
    std::scoped_lock l(hasDataItemsMutex);
    hasDataItems.emplace_back(item);
}

void MQTTController::unregisterHasDataItem(HasData *item) {
    std::scoped_lock l(hasDataItemsMutex);
    hasDataItems.erase(std::remove(hasDataItems.begin(), hasDataItems.end(), item), hasDataItems.end());
}
