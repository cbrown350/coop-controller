#include "MQTTController.h"

#include "coop_settings.h"
#include <WiFiManager.h>
#include <vector>
#include <thread>

#include "CoopLogger.h"
#include <MQTT.h>


// namespace MQTTController {
    using std::vector;
    using std::thread;
    
    // const char * const TAG = "mqtt";
    // WiFiClient net;
    // MQTTClient client;

    // WiFiManagerParameter mqtt_server("mqtt_server", "MQTT Server", DEFAULT_MQTT_SERVER, 40);
    // WiFiManagerParameter mqtt_port("mqtt_port", "MQTT Port", DEFAULT_MQTT_PORT, 40);
    // WiFiManagerParameter mqtt_user("mqtt_user", "MQTT User", DEFAULT_MQTT_USER, 40);
    // WiFiManagerParameter mqtt_password("mqtt_password", "MQTT Password", DEFAULT_MQTT_PASSWORD, 40);
    
    // vector<WiFiManagerParameter*> settings{&mqtt_server, &mqtt_port, &mqtt_user, &mqtt_password};

    // vector<WiFiManagerParameter*>& getSettingParams() {
    //     return settings;
    // }

    void MQTTController::connect() {
        CoopLogger::logi(TAG, "Will attempt to connect once wifi is established...");
        while (WiFi.status() != WL_CONNECTED) {
            // CoopLogger::getDefaultPrintStream()->print("-m-");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        CoopLogger::logi(TAG, "\nWifi on, connecting to MQTT at %s...", HOSTNAME);
        while (!client.connect(HOSTNAME, mqtt_user.getValue(), mqtt_password.getValue())) {
            CoopLogger::getDefaultPrintStream()->print("-mq-");
            std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }

        CoopLogger::logi(TAG, "\nMQTT connected!");

        client.subscribe("/hello");
        // client.unsubscribe("/hello");
    }

    void MQTTController::messageReceived(String &topic, String &payload) {
        CoopLogger::logi(TAG, "incoming: %s - %s", topic, payload);

        // Note: Do not use the client in the callback to publish, subscribe or
        // unsubscribe as it may cause deadlocks when other things arrive while
        // sending and receiving acknowledgments. Instead, change a global variable,
        // or push to a queue and handle it in the loop after calling `client.loop()`.
    }

    void MQTTController::mqttLoop() {
        connect();
        while(true) {
            client.loop();

            if (!client.connected()) 
                connect();

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            client.publish("/hello", "world");
        }
    }

    void MQTTController::init() {
        client.begin(mqtt_server.getValue(), atoi(mqtt_port.getValue()), net);
        client.onMessage([this](String &topic, String &payload) { messageReceived(topic, payload); });

        thread t([this](){ mqttLoop(); });
        t.detach();
    }
// }