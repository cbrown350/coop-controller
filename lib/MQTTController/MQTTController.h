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


#ifdef DEFAULT_MQTT_SERVER
#define DEFAULT_MQTT_SERVER_DEFAULT DEFAULT_MQTT_SERVER
#else
#define DEFAULT_MQTT_SERVER_DEFAULT ""
#endif

class MQTTController : public HasData<>,
                        public cwifi::ConfigWithWiFi {
    public:
        inline static constexpr const char * const TAG{"mqtt"};

        // Data keys
        inline static constexpr const char * const MQTT_SERVER = "mqtt_server";
        inline static constexpr const char * const MQTT_PORT = "mqtt_port";
        inline static constexpr const char * const MQTT_USER = "mqtt_user";
        inline static constexpr const char * const MQTT_PASSWORD = "mqtt_password";

        explicit MQTTController(const std::string &instanceID, std::string topicPrefix = MQTT_TOPIC_PREFIX) :
                            HasData(instanceID),
                            topicPrefix(std::move(topicPrefix)) {
            static int controllerIDCount{0};
            controllerID = controllerIDCount++;

            // Required c-string copies for WiFiManagerParameter since it doesn't seem to copy in the c-strings
            strcpy(mqtt_server_param_id, getValueWithID(MQTT_SERVER).c_str());
            strcpy(mqtt_server_param_label, getValueWithID("MQTT Server", true).c_str());
            strcpy(mqtt_port_param_id, getValueWithID(MQTT_PORT).c_str());
            strcpy(mqtt_port_param_label, getValueWithID("MQTT Port", true).c_str());
            strcpy(mqtt_user_param_id, getValueWithID(MQTT_USER).c_str());
            strcpy(mqtt_user_param_label, getValueWithID("MQTT User", true).c_str());
            strcpy(mqtt_password_param_id, getValueWithID(MQTT_PASSWORD).c_str());
            strcpy(mqtt_password_param_label, getValueWithID("MQTT Password", true).c_str());
        }

        ~MQTTController() override;

        [[nodiscard]] const char * getTag() const override { return TAG; }
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

        inline static const std::vector<std::string> nvsDataKeys = {MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD};
        inline static const std::vector<std::string> readOnlyKeys{};
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, nvsDataKeys, true);

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

        std::string getValueWithID(const char * name, bool addSpace = false) const {
            std::string value = name;
            if(controllerID == 0)
                return value;
            if (addSpace)
                value.append(" ");
            value.append(std::to_string(controllerID));
            return value;
        }

        char mqtt_server_param_id[std::char_traits<char>::length(MQTT_SERVER)+2] = "";
        char mqtt_server_param_label[std::char_traits<char>::length(MQTT_SERVER)+3] = "";
        WiFiManagerParameter mqtt_server_param{mqtt_server_param_id,
                                               mqtt_server_param_label,
                                                DEFAULT_MQTT_SERVER_DEFAULT,
                                                MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

        char mqtt_port_param_id[std::char_traits<char>::length(MQTT_PORT)+2] = "";
        char mqtt_port_param_label[std::char_traits<char>::length(MQTT_PORT)+3] = "";
        WiFiManagerParameter mqtt_port_param{mqtt_port_param_id,
                                             mqtt_port_param_label,
                                             DEFAULT_MQTT_PORT,
                                             MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

        char mqtt_user_param_id[std::char_traits<char>::length(MQTT_USER)+2] = "";
        char mqtt_user_param_label[std::char_traits<char>::length(MQTT_USER)+3] = "";
        WiFiManagerParameter mqtt_user_param{mqtt_user_param_id,
                                             mqtt_user_param_label,
                                             DEFAULT_MQTT_USER,
                                             MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

        char mqtt_password_param_id[std::char_traits<char>::length(MQTT_PASSWORD)+2] = "";
        char mqtt_password_param_label[std::char_traits<char>::length(MQTT_PASSWORD)+3] = "";
        WiFiManagerParameter mqtt_password_param{mqtt_password_param_id,
                                                 mqtt_password_param_label,
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
            if(key == MQTT_PORT) {
                std::unique_lock l{_dataMutex, std::defer_lock};
                if (!noLock)
                    l.lock();
                return std::to_string(mqttPort);
            }
            return getStringDataHelper({{MQTT_SERVER, mqttServer},
                                        {MQTT_USER, mqttUser},
//                                         {MQTT_PORT, mqttPort},
                                        {MQTT_PASSWORD, mqttPassword}},
                                    key, noLock);
        }

        [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value_raw, const bool noLock, const bool doObjUpdate) override {
            if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
                std::find(keys.begin(), keys.end(), key) == keys.end()) {
                Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
                return false;
            }
            const std::string value = utils::trim_clean(value_raw);

            std::unique_lock l{_dataMutex, std::defer_lock};

            std::string updatedKey;

            const auto portConverter = [](const std::string &key, std::any varPtr, const std::string &value) {
                Logger::logv(TAG, "[portConverter]");
//                return lookupESPTimezone(value) && setStringDataHelperStringSetter(varPtr, value);
                if(value.empty() || value == HasData::EMPTY_VALUE || !utils::isPositiveNumber(value)) {
                    Logger::loge(TAG, "Invalid port %s, not saving %s", value.c_str(), key.c_str());
                    return false;
                }
                auto numStoreValue = (unsigned)std::stoi(value);
                if(numStoreValue > 65535 || numStoreValue <= 0) {
                    Logger::loge(TAG, "Invalid port %s, not saving %s", value.c_str(), key.c_str());
                    return false;
                }
                unsigned &var = *std::any_cast<unsigned*>(varPtr);
                if(var != numStoreValue) {
                    var = numStoreValue;
                    return true;
                }
                return false;
            };
            if(updatedKey.empty() && key != MQTT_PORT) {
                updatedKey = setStringDataHelper({
                                                 {MQTT_SERVER,   {&mqttServer}},
                                                 {MQTT_USER,     {&mqttUser}},
                                                 {MQTT_PORT, {&mqttPort, portConverter}},
                                                 {MQTT_PASSWORD, {&mqttPassword}}},
                                                            key, value, noLock || l.owns_lock());
            }

            static std::vector<std::string> keysToUpdateOnObj = {};
            bool updated = !updatedKey.empty();
            if(updated) {
                if (doObjUpdate) {
                    if (!noLock && !l.owns_lock())
                        l.lock();

                    keysToUpdateOnObj.push_back(updatedKey);
                }
            }

            if(doObjUpdate && !keysToUpdateOnObj.empty()) {
                const bool objUpdated = updateObj(keysToUpdateOnObj, noLock || l.owns_lock()) || updated; // include updated since vars were updated
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