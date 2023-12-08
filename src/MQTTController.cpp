#include "MQTTController.h"
#include "coop_settings.h"
#include <WiFiManager.h>
#include <vector>
#include <thread>
#include "utils.h"

#include "CoopLogger.h"
#include <MQTT.h>


using std::vector;
using std::thread;

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE


void MQTTController::registerSubscriptions() {
    client.subscribe("/topic");
}

void MQTTController::publishData() {
    client.publish("/topic", "message", true, 0);
}

void MQTTController::messageReceived(String &topic, String &payload) {
    CoopLogger::logi(TAG, "incoming: %s - %s", topic.c_str(), payload.c_str());

    // Note: Do not use the client in the callback to publish, subscribe or
    // unsubscribe as it may cause deadlocks when other things arrive while
    // sending and receiving acknowledgments. Instead, change a global variable,
    // or push to a queue and handle it in the loop after calling `client.loop()`.
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

    strcpy(clientID, std::string(HOSTNAME "-" + std::to_string(controllerID)).c_str());
    CoopLogger::logi(TAG, "Wifi connected, %s connecting to MQTT on %s:%s as user %s...", clientID, mqtt_server.getValue(), mqtt_port.getValue(), mqtt_user.getValue());
    while (!client.connect(clientID, mqtt_user.getValue(), mqtt_password.getValue()) && 
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
    while(utils::wait_for<std::chrono::seconds>(std::chrono::seconds(1), loopThreadMutex, loopThreadCond, loopThreadStop)) {
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
    const char* server = mqtt_server.getValue();
    if (strcmp(server, "") == 0) {
        CoopLogger::loge(TAG, "MQTT (%d) server not set", controllerID);
        return;
    }

    IPAddress ip;
    if(ip.fromString(server)) {
        net = new WiFiClient();
        client.begin(ip, atoi(mqtt_port.getValue()), *net);
    } else if(strcmp(mqtt_port.getValue(), "1883") == 0) { // assume insecure if standard port, otherwise will require addl handling
        net = new WiFiClient();
        client.begin(server, atoi(mqtt_port.getValue()), *net);
    } else {
        net = new WiFiClientSecure();
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE    
        ((WiFiClientSecure*)net)->setCACertBundle(x509_crt_imported_bundle_bin_start);
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE   
        client.begin(server, atoi(mqtt_port.getValue()), *net);
    }

    client.onMessage([this](String &topic, String &payload) { messageReceived(topic, payload); });

    loopThreadStop = false;
    loopThread = new thread([this](){ mqttLoop(); });
    CoopLogger::logi(TAG, "MQTT (%d) started", controllerID);
}

void MQTTController::deinit() {
    if(loopThread) {
        {
            std::scoped_lock<std::mutex> l(loopThreadMutex);
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
    deinit();
}

vector<WiFiManagerParameter *> MQTTController::getSettingParams() {
    mqtt_server.setValue(_data["mqtt_server"].c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
    mqtt_port.setValue(_data["mqtt_port"].c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
    mqtt_user.setValue(_data["mqtt_user"].c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
    mqtt_password.setValue(_data["mqtt_password"].c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
    return {&mqtt_server, &mqtt_port, &mqtt_user, &mqtt_password};
}

void MQTTController::afterConfigPageSave() {
    _data.emplace("mqtt_server", mqtt_server.getValue());
    _data.emplace("mqtt_port", mqtt_port.getValue());
    _data.emplace("mqtt_user", mqtt_user.getValue());
    _data.emplace("mqtt_password", mqtt_password.getValue());
}
