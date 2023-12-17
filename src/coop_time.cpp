#include "coop_time.h"
#include <CSV_Parser.h>

extern const char zones_csv_start[] asm("_binary_zones_csv_start");
extern const char zones_csv_end[]   asm("_binary_zones_csv_end");

namespace coop_time {
    static constexpr const char * const TAG = "ctime";

    class : public HasData<> {
    public:
        using HasData<>::getData;
        using HasData<>::get;
        std::map<std::string, std::string> getData() override {
            get("*");
            return HasData<>::getData();
        }
        std::string get(const std::string &key) override {
            std::scoped_lock l{_dataMutex};
            if(key == "*" || key == "uptime") {
                _data[key] = getUptime();
            }
            if(key == "*" || key == "current_time") {
                _data[key] = getCurrentTime();
            }
            if(key == "*" || key == "free_memory") {
                _data[key] = getFreeMemory();
            }
            if(key != "*")
                return _data[key];
            return "";
        }
    } data;

    std::map<std::string, std::string> getData() {
        return data.getData();
    }
    std::string get(const std::string &key) {
        return data.get(key);
    }

    WiFiManagerParameter timezone{"timezone",
                                  "Timezone",
                                  DEFAUlT_TIMEZONE,
                                  MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
    WiFiManagerParameter ntp_server1{"ntp_server1",
                                     "NTP Server 1",
                                     NTP_SERVER1_DEFAULT,
                                     MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
    WiFiManagerParameter ntp_server2{"ntp_server2",
                                     "NTP Server 2",
                                     NTP_SERVER2_DEFAULT,
                                     MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};
    WiFiManagerParameter ntp_server3{"ntp_server3",
                                     "NTP Server 3",
                                     NTP_SERVER3_DEFAULT,
                                     MAX_WIFI_EXTRA_PARAM_MAX_LENGTH};

    void lookupESPTimezone(const std::string &regularTimezone, char * espTimezone) {
        CSV_Parser cp(zones_csv_start, /*format*/ "ss", false);
        char **regularTimezoneFields = (char**)cp[0];
        char **espTimezoneFields = (char**)cp[1];
        for(int row_index = 0; row_index < cp.getRowsCount(); row_index++) {
//            CoopLogger::logv(TAG, "[lookupESPTimezone] regularTimezoneFields[%d]: %s, espTimezoneFields[%d]: %s", row_index, regularTimezoneFields[row_index], row_index, espTimezoneFields[row_index]);
            if(strcmp(regularTimezoneFields[row_index], regularTimezone.c_str()) == 0) {
                strcpy(espTimezone, espTimezoneFields[row_index]);
                CoopLogger::logv(TAG, "[lookupESPTimezone] Found timezone %s->%s in zones.csv", regularTimezone.c_str(), espTimezone);
            }
        }
    }

    void setTimezoneAndNTP() {
        static char espTimezone[50] = "";

        std::map<std::string, std::string> &_data = data.getDataRef();
        const char * _timezone = _data["timezone"].c_str();
        const char * _ntp_server1 = _data["ntp_server1"].c_str();
        const char * _ntp_server2 = _data["ntp_server2"].c_str();
        const char * _ntp_server3 = _data["ntp_server3"].c_str();
        lookupESPTimezone(_timezone, espTimezone);

        setenv("TZ", espTimezone,1);
        tzset();
        if(espTimezone[0] != '\0') {
            configTzTime(espTimezone, _ntp_server1, _ntp_server2, _ntp_server3);
        } else {
            CoopLogger::logw(TAG, "[setTimezoneAndNTP] Timezone %s not found in zones.csv; couldn't set _timezone", _timezone);
            configTime(0, 0, _ntp_server1, _ntp_server2, _ntp_server3);
        }

        CoopLogger::logv(TAG, "[setTimezoneAndNTP] timezone: %s (%s)", _timezone, espTimezone);
        CoopLogger::logv(TAG, "[setTimezoneAndNTP] ntp_server1: %s", _ntp_server1);
        CoopLogger::logv(TAG, "[setTimezoneAndNTP] ntp_server2: %s", _ntp_server2);
        CoopLogger::logv(TAG, "[setTimezoneAndNTP] ntp_server3: %s", _ntp_server3);
        CoopLogger::logv(TAG, "[setTimezoneAndNTP] current time: %s", getCurrentTime().c_str());
    }

    std::string& getUptime() {
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

    std::string& getCurrentTime() {
        static std::string time_str;
        auto now = time(nullptr);
        time_str = ctime(&now);
        return time_str;
    }

    void updateWiFiParamsFromDataVars(std::vector<WiFiManagerParameter *> params) {
        CoopLogger::logv(TAG, "[updateWiFiParamsFromDataVars]");
        std::map<std::string, std::string> &_data = data.getDataRef();
        for(auto &setting : params) {
            std::string &value = _data[setting->getID()];
            if(!value.empty())
                setting->setValue(value.c_str(), MAX_WIFI_EXTRA_PARAM_MAX_LENGTH);
        }
    }

    void updateDataVarsFromWifiParams() {
        CoopLogger::logv(TAG, "[updateDataVarsFromWifiParams]");
//        data.set("timezone", timezone.getValue());
//        data.set("ntp_server1", ntp_server1.getValue());
//        data.set("ntp_server2", ntp_server2.getValue());
//        data.set("ntp_server3", ntp_server3.getValue());
        for(auto &setting : getWifiConfig().getSettingParams()) {
            data.set(setting->getID(), setting->getValue());
        }
    }

    class : public coop_wifi::ConfigWithWiFi {
    public:
        [[nodiscard]] std::vector<WiFiManagerParameter*> getSettingParams() override {
            CoopLogger::logv(TAG, "[getSettingParams]");
            auto params = {&timezone, &ntp_server1, &ntp_server2, &ntp_server3};
            updateWiFiParamsFromDataVars(params);
            return params;
        }
        void afterConfigPageSave() override {
            updateDataVarsFromWifiParams();
            setTimezoneAndNTP();
        }
    } wifiConfig;

    coop_wifi::ConfigWithWiFi & getWifiConfig() {
        return wifiConfig;
    }

    void init() {
        CoopLogger::logv(TAG, "[init]");
        updateDataVarsFromWifiParams();
//        setTimezoneAndNTP();
    }

    std::string & getFreeMemory() {
        static std::string free_memory;
        free_memory = std::to_string(ESP.getFreeHeap());
        return free_memory;
    }

} // namespace coop_time