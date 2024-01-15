#include "settings.h"

#include "ntp_time.h"

#include <cwifi.h>
#include <utils.h>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <functional>

#include <CSV_Parser.h>

extern const char zones_csv_start[] asm("_binary_zones_csv_start");
//extern const char zones_csv_end[]   asm("_binary_zones_csv_end");

namespace ntp_time {

    static std::string timezone = DEFAULT_TIMEZONE;
    static std::string ntpServer1 = NTP_SERVER1_DEFAULT;
//    static std::string ntpServer2 = NTP_SERVER2_DEFAULT;
//    static std::string ntpServer3 = NTP_SERVER3_DEFAULT;

    bool setTimezoneAndNTP();

    namespace {
        class _ : public HasData<> {
            public:
                _ () : HasData("time") { };
                using HasData::getNvsNamespace;
                using HasData::loadNvsData;
                using HasData::saveNvsData;
                using HasData::deleteNvsData;
                using HasData::getData;
                using HasData::setData;
                using HasData::get;
                using HasData::set;

                [[nodiscard]] const char * getTag() const override { return TAG; }

                [[nodiscard]] std::vector<std::string> getNvsKeys() const override { return ntp_time::nvsDataKeys; }
                [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
                [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }

            private:
                [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
//                    std::unique_lock l{_dataMutex, std::defer_lock};
//                    switch(utils::hashstr(key.c_str())) {
//                        case utils::hashstr(UPTIME): {
//                            if(!noLock)
//                                l.lock();
//                            return getUptime();
//                        }
//                        case utils::hashstr(CURRENT_TIME): {
//                            if(!noLock)
//                                l.lock();
//                            return getCurrentTime();
//                        }
//                        case utils::hashstr(FREE_MEMORY): {
//                            if(!noLock)
//                                l.lock();
//                            return getFreeMemory();
//                        }
//                        case utils::hashstr(TIMEZONE): {
//                            if(!noLock)
//                                l.lock();
//                            return timezone;
//                        }
//                        case utils::hashstr(NTP_SERVER1): {
//                            if(!noLock)
//                                l.lock();
//                            return ntpServer1;
//                        }
////                        case utils::hashstr(NTP_SERVER2):
////                        case utils::hashstr(NTP_SERVER3):
//                        default: {}
//                    }
//                    Logger::logw(TAG, "Invalid key %s", key.c_str());
//                    return HasData::EMPTY_VALUE;std::function<std::string()>

                    using retType = std::string;
                    using FunPtrType = retType(*)();
                    return getStringDataHelper<FunPtrType>({
                                       {UPTIME,       (FunPtrType)[]()->retType { return getUptime(); } },
                                       {CURRENT_TIME, (FunPtrType)[]()->retType { return getCurrentTime(); } },
                                       {FREE_MEMORY,  (FunPtrType)[]()->retType { return getFreeMemory(); } },
                                       {TIMEZONE,     (FunPtrType)[]()->retType { return timezone; } },
                                       {NTP_SERVER1,  (FunPtrType)[]()->retType { return ntpServer1; } }
                                       },
                                   key, noLock, []()->retType { return HasData::EMPTY_VALUE; })();
//                    return getStringDataHelper({
//                                       {UPTIME,       getUptime()},
//                                       {CURRENT_TIME, getCurrentTime()},
//                                       {FREE_MEMORY,  getFreeMemory()},
//                                       {TIMEZONE,     timezone},
//                                       {NTP_SERVER1,  ntpServer1}
//                                       },
//                                   key, noLock);
                }

                [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value,
                                                           const bool noLock, const bool doObjUpdate) override {
                    if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
                        std::find(keys.begin(), keys.end(), key) == keys.end()) {
                        Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
                        return false;
                    }

                    static std::vector<std::string> keysToUpdateOnObj = {};
//                    bool updated = false;
//
//                    std::unique_lock l{_dataMutex, std::defer_lock};
//                    switch(utils::hashstr(key.c_str())) {
//                        case utils::hashstr(TIMEZONE): {
//                            if(!noLock)
//                                l.lock();
//                            updated = timezone != value;
//                            if(updated) {
//                                timezone = value;
//                                if (doObjUpdate)
//                                    keysToUpdateOnObj.push_back(key);
//                            }
//                            break;
//                        }
//                        case utils::hashstr(NTP_SERVER1): {
//                            if(!noLock)
//                                l.lock();
//                            updated = ntpServer1 != value;
//                            if(updated) {
//                                ntpServer1 = value;
//                                if (doObjUpdate)
//                                    keysToUpdateOnObj.push_back(key);
//                            }
//                            break;
//                        }
////                        case utils::hashstr(NTP_SERVER2):
////                        case utils::hashstr(NTP_SERVER3):
//                        default: {
//                            Logger::logw(TAG, "Invalid key %s", key.c_str());
//                            return false;
//                        }
//                    }
                    // TODO: check values are valid
                    const auto updatedKey = setStringDataHelper({{TIMEZONE,   timezone},
                                                                                    {NTP_SERVER1, ntpServer1}},
                                                                            key, value, noLock);
                    bool updated = !updatedKey.empty();
                    if(updated) {
                        if (doObjUpdate) {
                            keysToUpdateOnObj.push_back(updatedKey);
                        }
                    }

                    if(doObjUpdate && !keysToUpdateOnObj.empty()) {
                        const bool objUpdated = updateObj(keysToUpdateOnObj, noLock) || updated; // include updated since vars were updated
                        if(objUpdated) {
                            Logger::logv(TAG, "Updated %s", utils::join(keysToUpdateOnObj, ", ").c_str());
                            keysToUpdateOnObj.clear();
                        }
                        return objUpdated;
                    }
                    return updated;
                }

                bool updateObj(const std::vector<std::string> &keys, const bool _dataAlreadyLocked) override {
                    // member vars already set in setWithOptLockAndUpdate(), use them to set timezone and server
                    bool success = setTimezoneAndNTP();
                    if(success) // don't save if new time values don't work
                        success &= HasData::updateObj(keys, _dataAlreadyLocked);
                    return success;
                }

            } data;
    }

    HasData<> &getData() {
        return data;
    }
    std::string get(const std::string &key) {
        return data.get(key);
    }

    WiFiManagerParameter timezone_param{TIMEZONE,
                                  "Timezone",
                                  DEFAULT_TIMEZONE,
                                  MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
    WiFiManagerParameter ntp_server1_param{NTP_SERVER1,
                                     "NTP Server 1",
                                     NTP_SERVER1_DEFAULT,
                                     MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
//    WiFiManagerParameter ntp_server2_param{NTP_SERVER2,
//                                     "NTP Server 2",
//                                     NTP_SERVER2_DEFAULT,
//                                     MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
//    WiFiManagerParameter ntp_server3_param{NTP_SERVER3,
//                                     "NTP Server 3",
//                                     NTP_SERVER3_DEFAULT,
//                                     MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

    void lookupESPTimezone(const std::string &regularTimezone, char * espTimezone) {
        CSV_Parser cp(zones_csv_start, /*format*/ "ss", false);
        char **regularTimezoneFields = (char**)cp[0];
        char **espTimezoneFields = (char**)cp[1];
        for(int row_index = 0; row_index < cp.getRowsCount(); row_index++) {
//            Logger::logv(TAG, "[lookupESPTimezone] regularTimezoneFields[%d]: %s, espTimezoneFields[%d]: %s",
//                          row_index, regularTimezoneFields[row_index], row_index, espTimezoneFields[row_index]);
            if(strcmp(regularTimezoneFields[row_index], regularTimezone.c_str()) == 0) {
                strcpy(espTimezone, espTimezoneFields[row_index]);
                Logger::logv(TAG, "[lookupESPTimezone] Found timezone %s->%s in zones.csv",
                                                        regularTimezone.c_str(), espTimezone);
            }
        }
    }

    bool setTimezoneAndNTP() {
        static char espTimezone[50] = "";
        bool success = true;

        const char * _timezone = timezone.c_str();
        const char * _ntp_server1 = ntpServer1.c_str();
//        const char * _ntp_server2 = ntpServer2.c_str();
//        const char * _ntp_server3 = ntpServer3.c_str();
        lookupESPTimezone(_timezone, espTimezone);

        // note: second and third servers are not used by ESP32 in configTzTime function call
        if(espTimezone[0] != '\0') {
            configTzTime(espTimezone, _ntp_server1, "_ntp_server2", "_ntp_server3");
        } else {
            Logger::logw(TAG, "[setTimezoneAndNTP] Timezone %s not found in zones.csv; couldn't set _timezone", _timezone);
            configTime(0, 0, _ntp_server1, "_ntp_server2", "_ntp_server3");
            success = false;
        }

        Logger::logv(TAG, "[setTimezoneAndNTP] timezone: %s (%s)", _timezone, espTimezone);
        Logger::logv(TAG, "[setTimezoneAndNTP] ntp_server1: %s", _ntp_server1);
//        Logger::logv(TAG, "[setTimezoneAndNTP] ntp_server2: %s", _ntp_server2);
//        Logger::logv(TAG, "[setTimezoneAndNTP] ntp_server3: %s", _ntp_server3);
        Logger::logv(TAG, "[setTimezoneAndNTP] current time: %s", getCurrentTime().c_str());

        return success;
    }

    bool update() {
        return setTimezoneAndNTP();
    }

    std::string& getUptime() { // TODO: dataMux lock?
        static std::string uptime_str;
        auto now = esp_timer_get_time();
        auto uptime = now / 1000000;
        long uptime_ms = (long)now % 1000000;
        int uptime_s = (int)(uptime % 60);
        int uptime_m = (int)(uptime / 60) % 60;
        int uptime_h = (int)(uptime / 3600) % 24;
        int uptime_d = (int)(uptime / 86400);
        uptime_str = utils::string_format("%dd %02d:%02d:%02d.%06ld", uptime_d, uptime_h, uptime_m, uptime_s, uptime_ms);
        return uptime_str;
    }

    std::string& getCurrentTime() { // TODO: dataMux lock?
        static std::string time_str;
        auto now = time(nullptr);
        time_str = ctime(&now);
        return time_str;
    }

    [[nodiscard]] std::vector<WiFiManagerParameter*> getSettingParamsNoUpdate() {
        Logger::logv(TAG, "[getSettingParamsNoUpdate]");
        return {&timezone_param, &ntp_server1_param}; //, &ntp_server2_param, &ntp_server3_param};
    }

    void updateDataVarsFromWifiParams() {
        Logger::logv(TAG, "[updateDataVarsFromWifiParams]");
        std::map<std::string, std::string> dataToUpdate = {};
        for(auto &setting : getSettingParamsNoUpdate())
            dataToUpdate[setting->getID()] = setting->getValue();
        if(!data.setData(dataToUpdate))
            Logger::loge(TAG, "Failed to save data from WiFi params");
    }

    void updateWiFiParamsFromDataVars() {
        Logger::logv(TAG, "[updateWiFiParamsFromDataVars]");
        for(auto &setting : getSettingParamsNoUpdate()) {
            std::string value = get(setting->getID());
            if(!value.empty())
                setting->setValue(value.c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
        }
    }

    class : public cwifi::ConfigWithWiFi {
    public:
        [[nodiscard]] std::vector<WiFiManagerParameter*> getSettingParams() override {
            Logger::logv(TAG, "[getSettingParams]");
            updateWiFiParamsFromDataVars();
            return getSettingParamsNoUpdate();
        }
        void afterConfigPageSave() override {
            Logger::logv(TAG, "[afterConfigPageSave]");
            updateDataVarsFromWifiParams();
        }
    } wifiConfig;

    cwifi::ConfigWithWiFi & getWifiConfig() {
        return wifiConfig;
    }

    void init() {
        Logger::logv(TAG, "[init]");
        if(!data.loadNvsData())
            Logger::loge(TAG, "Failed to load data from NVS");
        setTimezoneAndNTP();
    }

    std::string & getFreeMemory() { // TODO: dataMux lock?
        static std::string free_memory;
        free_memory = std::to_string(ESP.getFreeHeap());
        return free_memory;
    }

} // namespace