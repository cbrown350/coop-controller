#ifndef MQTT_CONTROLLER_H
#define MQTT_CONTROLLER_H


#include <MQTT.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>

#include <HasData.h>
#include <Logger.h>
#include <cwifi.h>

#include <utility>
#include <vector>
#include <string>
#include <thread>
#include <condition_variable>
#include <map>
#include <any>


class MQTTController : public HasData<>,
                        public cwifi::ConfigWithWiFi {
    public:
        inline static constexpr const char * TAG{"mqtt"};

        // Data keys
        inline static constexpr const char * const MQTT_SERVER = "mqtt_server";
        inline static constexpr const char * const MQTT_PORT = "mqtt_port";
        inline static constexpr const char * const MQTT_USER = "mqtt_user";
        inline static constexpr const char * const MQTT_PASSWORD = "mqtt_password";

        inline static const std::vector<std::string> nvsDataKeys = { MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD };
        inline static const std::vector<std::string> readOnlyKeys{};
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, nvsDataKeys, true);

        explicit MQTTController(const std::string &instanceID, std::string topicPrefix = MQTT_TOPIC_PREFIX) :
                            HasData(instanceID),
                            topicPrefix(std::move(topicPrefix)) {
            static int controllerIDCount{0};
            controllerID = controllerIDCount++;
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
        // cwifi::ConfigWithWiFi
        [[nodiscard]] std::vector<WiFiManagerParameter *> getSettingParams() override;
        void afterConfigPageSave() override;

        void registerHasDataItem(HasData *item);
        void unregisterHasDataItem(HasData *item);

        // HasData
        using HasData::loadNvsData;
        using HasData::saveNvsData;
        using HasData::deleteNvsData;
        using HasData::getData;
        using HasData::setData;
        using HasData::get;
        using HasData::set;
        [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
        [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }
        [[nodiscard]] std::vector<std::string> getNvsKeys() const override { return nvsDataKeys; }

    private:
        int controllerID;

        WiFiClient *net = nullptr;
        MQTTClient client{MQTT_MAX_PACKET};
        char clientID[MAX_HOSTNAME_LENGTH+3] = {0}; // client ID length is limited by the MQTT spec

        std::string topicPrefix;
        std::string lastWillTopic;
        std::string offlineMsg;
        std::string onlineMsg;
        
        std::condition_variable loopThreadCond;
        std::mutex loopThreadMutex;
        std::thread *loopThread = nullptr;
        bool loopThreadStop = true;

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
        WiFiManagerParameter mqtt_server_param{getValueWithID(MQTT_SERVER),
                                         getValueWithID("MQTT Server", true),
                                         DEFAULT_MQTT_SERVER_DEFAULT,
                                         MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_port_param{getValueWithID(MQTT_PORT),
                                       getValueWithID("MQTT Port", true),
                                       DEFAULT_MQTT_PORT,
                                       MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_user_param{getValueWithID(MQTT_USER),
                                       getValueWithID("MQTT User", true),
                                       DEFAULT_MQTT_USER,
                                       MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
        WiFiManagerParameter mqtt_password_param{getValueWithID(MQTT_PASSWORD),
                                           getValueWithID("MQTT Password", true),
                                           DEFAULT_MQTT_PASSWORD,
                                           MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

        std::string mqttServer = DEFAULT_MQTT_SERVER_DEFAULT;
        unsigned mqttPort = std::stoi(DEFAULT_MQTT_PORT);
        std::string mqttUser = DEFAULT_MQTT_USER;
        std::string mqttPassword = DEFAULT_MQTT_PASSWORD;

        void messageReceived(String &topic, String &payload);
        void connect();
        void disconnect();
        void mqttLoop();
        void publishData();
        void registerSubscriptions();
        bool restart();
        std::string getTopicPrefix() const { return topicPrefix; }

        std::mutex hasDataItemsMutex;
        std::vector<HasData *> hasDataItems;

        // HasData
        using HasData::updateObj;

        [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
            std::unique_lock l{_dataMutex, std::defer_lock};
            switch(utils::hashstr(key.c_str())) {
                case utils::hashstr(MQTT_SERVER): {
                    if(!noLock)
                        l.lock();
                    return mqttServer;
                }
                case utils::hashstr(MQTT_PORT): {
                    if(!noLock)
                        l.lock();
                    return std::to_string(mqttPort);
                }
                case utils::hashstr(MQTT_USER): {
                    if(!noLock)
                        l.lock();
                    return mqttUser;
                }
                case utils::hashstr(MQTT_PASSWORD): {
                    if(!noLock)
                        l.lock();
                    return mqttPassword;
                }
                default: {}
            }
            Logger::logw(TAG, "Invalid key %s", key.c_str());
            return HasData::EMPTY_VALUE;
        }


        [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, const bool noLock, const bool doObjUpdate) override {
            if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
                std::find(keys.begin(), keys.end(), key) == keys.end()) {
                Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
                return false;
            }

            static std::vector<std::string> keysToUpdateOnObj = {};
            bool updated = false;

            std::unique_lock l{_dataMutex, std::defer_lock};
            switch(utils::hashstr(key.c_str())) {
                case utils::hashstr(MQTT_SERVER): {
                    if(!noLock)
                        l.lock();
                    mqttServer = value;
                    updated = true;
                    keysToUpdateOnObj.push_back(key);
                    break;
                }
                case utils::hashstr(MQTT_PORT): {
                    if(value.empty() || value == HasData::EMPTY_VALUE || !utils::isPositiveNumber(value)) {
                        Logger::loge(TAG, "Invalid port %s, not saving %s", value.c_str(), key.c_str());
                        return false;
                    }
                    auto numStoreValue = std::stoi(value);
                    if(numStoreValue > 65535 || numStoreValue <= 0) {
                        Logger::loge(TAG, "Invalid port %s, not saving %s", value.c_str(), key.c_str());
                        return false;
                    }
                    if(!noLock)
                        l.lock();
                    mqttPort = numStoreValue;
                    updated = true;
                    keysToUpdateOnObj.push_back(key);
                    break;
                }
                case utils::hashstr(MQTT_USER): {
                    if(!noLock)
                        l.lock();
                    mqttUser = value;
                    updated = true;
                    keysToUpdateOnObj.push_back(key);
                    break;
                }
                case utils::hashstr(MQTT_PASSWORD): {
                    if(!noLock)
                        l.lock();
                    mqttPassword = value;
                    updated = true;
                    keysToUpdateOnObj.push_back(key);
                    break;
                }
                default: {}
            }
            if(doObjUpdate && !keysToUpdateOnObj.empty()) {
                const bool objUpdated = updateObj(keysToUpdateOnObj, l.owns_lock());
                if(objUpdated) {
                    Logger::logv(TAG, "Updated %s", utils::join(keysToUpdateOnObj, ", ").c_str());
                    keysToUpdateOnObj.clear();
                }
                return objUpdated;
            }
            return updated;
        }

        bool updateObj(const std::vector<std::string> &_keys, const bool _dataAlreadyLocked) override {
            // member vars already set in setWithOptLockAndUpdate(), use them in restart()
            bool success = restart() || true; // don't need to check if restarted worked, just need to update data
            if(success) // don't save if new time values don't work
                success &= HasData::updateObj(_keys, _dataAlreadyLocked); // save to NVS if keys not empty
            return success;
        }
};

#endif // MQTT_CONTROLLER_H