#ifndef MQTT_CONTROLLER_H
#define MQTT_CONTROLLER_H


#include <MQTT.h>
#include <WiFiManager.h>
#include <vector>
#include "coop_settings.h"
#include <string>
#include <thread>
#include <condition_variable>
#include <WiFiClientSecure.h>
#include "HasData.h"
#include "coop_wifi.h"
#include <map>
#include <any>


class MQTTController : public HasData<>, public coop_wifi::HasConfigPageParams {
    private:    
        static constexpr const char * TAG{"mqtt"};

        inline static int controllerIDCount = 0;      

        int controllerID = controllerIDCount++;  

        WiFiClient *net = nullptr;
        MQTTClient client{MQTT_MAX_PACKET};
        char clientID[MAX_HOSTNAME_LENGTH+3] = {0}; // client ID length is limited by the MQTT spec

        std::string lastWillTopic;
        std::string offlineMsg;
        std::string onlineMsg;
        
        std::condition_variable loopThreadCond;
        std::mutex loopThreadMutex;
        std::thread *loopThread = nullptr;
        bool loopThreadStop = false;

        const char * getValueWithID(const char * name, bool addSpace = false) const {
            if(controllerID == 0) {
                return name;
            }
            if (addSpace) {
                return (std::string(name) + " " + std::to_string(controllerID)).c_str();
            } else {
                return (name + std::to_string(controllerID)).c_str();
            }
        }

        WiFiManagerParameter mqtt_server{getValueWithID("mqtt_server"),
                                         getValueWithID("MQTT Server", true),
                                         nullptr,
                                         MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_port{getValueWithID("mqtt_port"),
                                       getValueWithID("MQTT Port", true),
                                       nullptr,
                                       MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_user{getValueWithID("mqtt_user"),
                                       getValueWithID("MQTT User", true),
                                       nullptr,
                                       MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_password{getValueWithID("mqtt_password"),
                                           getValueWithID("MQTT Password", true),
                                           nullptr,
                                           MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

        void messageReceived(String &topic, String &payload);
        void connect();
        void disconnect();
        void mqttLoop();
        void publishData();
        void registerSubscriptions();

    public:
        void setupDataVars() {
//#ifdef DEFAULT_MQTT_SERVER
//            _data["mqtt_server"] = DEFAULT_MQTT_SERVER;
//#else
//            _data["mqtt_server"] = "";
//#endif
//            _data["mqtt_port"] = DEFAULT_MQTT_PORT;
//            _data["mqtt_user"] = DEFAULT_MQTT_USER;
//            _data["mqtt_password"] = DEFAULT_MQTT_PASSWORD;

#ifdef DEFAULT_MQTT_SERVER
            setData("mqtt_server", DEFAULT_MQTT_SERVER);
#else
            setData("mqtt_server", "");
#endif
            setData("mqtt_port", DEFAULT_MQTT_PORT);
            setData("mqtt_user", DEFAULT_MQTT_USER);
            setData("mqtt_password", DEFAULT_MQTT_PASSWORD);
        };
        MQTTController() {
            setupDataVars();
        };
        ~MQTTController() override;

        void init();
        void deinit();
        void setLastWill(const std::string &lastWillTopic, const std::string &offlineMsg = "offline", const std::string &onlineMsg = "online");
        void clearLastWill();

        // HasWiFiConfigPageParams
        [[nodiscard]] std::vector<WiFiManagerParameter *> getSettingParams() override;
        void afterConfigPageSave() override;
};

#endif // MQTT_CONTROLLER_H