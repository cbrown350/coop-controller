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


class MQTTController : public HasData, public coop_wifi::ConfigWithWiFi {
    public:
        static constexpr const char * TAG{"mqtt"};

        int controllerID;

        // Data keys
        inline static constexpr const char * const MQTT_SERVER = "mqtt_server";
        inline static constexpr const char * const MQTT_PORT = "mqtt_port";
        inline static constexpr const char * const MQTT_USER = "mqtt_user";
        inline static constexpr const char * const MQTT_PASSWORD = "mqtt_password";

        inline static const std::vector<std::string> readOnlyKeys{};

        explicit MQTTController(const std::string &instanceID) : HasData(instanceID) {
            HasData::readOnlyKeys = MQTTController::readOnlyKeys;
            static int controllerIDCount{0};
            controllerID = controllerIDCount++;

            updateDataVarsFromWifiParams();
        }
        ~MQTTController() override;

        void init();
        void startLoop();
        void stopLoop();
        void setLastWill(const std::string &lastWillTopic, const std::string &offlineMsg = "offline", const std::string &onlineMsg = "online");
        void clearLastWill();

        void updateDataVarsFromWifiParams();
        void updateWiFiParamsFromDataVars();

        [[nodiscard]] std::vector<WiFiManagerParameter *> getSettingParamsNoUpdate();
        // coop_wifi::ConfigWithWiFi
        [[nodiscard]] std::vector<WiFiManagerParameter *> getSettingParams() override;
        void afterConfigPageSave() override;

        void registerHasDataItem(HasData *item);
        void unregisterHasDataItem(HasData *item);

        // HasData
        [[nodiscard]] std::string getNvsNamespace() const override { return MQTTController::TAG; }
        [[nodiscard]] std::vector<std::string> getNvsKeys() const override {
            const static std::vector<std::string> ids = {
                    MQTT_SERVER,
                    MQTT_PORT,
                    MQTT_USER,
                    MQTT_PASSWORD
            };
            return ids;
        }



    private:
        std::mutex initMutex;

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

#ifdef DEFAULT_MQTT_SERVER
#define DEFAULT_MQTT_SERVER_DEFAULT DEFAULT_MQTT_SERVER
#else
#define DEFAULT_MQTT_SERVER_DEFAULT ""
#endif
        WiFiManagerParameter mqtt_server{getValueWithID(MQTT_SERVER),
                                         getValueWithID("MQTT Server", true),
                                         DEFAULT_MQTT_SERVER_DEFAULT,
                                         MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_port{getValueWithID(MQTT_PORT),
                                       getValueWithID("MQTT Port", true),
                                       DEFAULT_MQTT_PORT,
                                       MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_user{getValueWithID(MQTT_USER),
                                       getValueWithID("MQTT User", true),
                                       DEFAULT_MQTT_USER,
                                       MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_password{getValueWithID(MQTT_PASSWORD),
                                           getValueWithID("MQTT Password", true),
                                           DEFAULT_MQTT_PASSWORD,
                                           MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

        std::mutex hasDataItemsMutex;
        std::vector<HasData *> hasDataItems;

        void messageReceived(String &topic, String &payload);
        void connect();
        void disconnect();
        void mqttLoop();
        void publishData();
        void registerSubscriptions();
};

#endif // MQTT_CONTROLLER_H