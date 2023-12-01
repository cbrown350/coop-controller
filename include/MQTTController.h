#ifndef MQTT_CONTROLLER_H
#define MQTT_CONTROLLER_H


#include <MQTT.h>
#include <WiFiManager.h>
#include <vector>
#include "coop_settings.h"
#include <string>

class MQTTController {
    private:
        inline static int controllerIDCount = 0;      

        int controllerID = controllerIDCount++;  
    
        static constexpr const char * TAG{"mqtt"};
        WiFiClient net;
        MQTTClient client{};

        const char * getValueWithID(const char * name, bool addSpace = false) {
            if(controllerID == 0) {
                return name;
            }
            if (addSpace) {
                return (name + std::to_string(controllerID) + " ").c_str();
            } else {
                return (name + std::to_string(controllerID)).c_str();
            }
        }

        WiFiManagerParameter mqtt_server{getValueWithID("mqtt_server"), getValueWithID("MQTT Server", true), DEFAULT_MQTT_SERVER, 40};
        WiFiManagerParameter mqtt_port{getValueWithID("mqtt_port"), getValueWithID("MQTT Port", true), DEFAULT_MQTT_PORT, 40};
        WiFiManagerParameter mqtt_user{getValueWithID("mqtt_user"), getValueWithID("MQTT User", true), DEFAULT_MQTT_USER, 40};
        WiFiManagerParameter mqtt_password{getValueWithID("mqtt_password"), getValueWithID("MQTT Password", true), DEFAULT_MQTT_PASSWORD, 40};

        void messageReceived(String &topic, String &payload);
        void connect();
        void mqttLoop();

    public:
        void init();
        std::vector<WiFiManagerParameter*> getSettingParams() { return {&mqtt_server, &mqtt_port, &mqtt_user, &mqtt_password}; }
};

#endif // MQTT_CONTROLLER_H