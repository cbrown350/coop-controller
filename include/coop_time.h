#ifndef COOP_TIME_H
#define COOP_TIME_H

#include "HasData.h"
#include "coop_wifi.h"
#include "coop_settings.h"
#include "utils.h"

namespace coop_time {
  inline static constexpr const char * const TAG = "ctime";

    // Data keys
    inline static constexpr const char * const UPTIME = "uptime";
    inline static constexpr const char * const CURRENT_TIME = "current_time";
    inline static constexpr const char * const FREE_MEMORY = "free_memory";
    inline static constexpr const char * const TIMEZONE = "timezone";
    inline static constexpr const char * const NTP_SERVER1 = "ntp_server1";
    // second and third servers are currently not used by ESP32 in configTzTime function call
//    inline static constexpr const char * const NTP_SERVER2 = "ntp_server2";
//    inline static constexpr const char * const NTP_SERVER3 = "ntp_server3";

    /*
    *   get values, ex.: get("ssid");
    */

    inline static const std::vector<std::string> nvsDataKeys = {
        TIMEZONE,
        NTP_SERVER1,
//        NTP_SERVER2,
//        NTP_SERVER3
    };
    inline static const std::vector<std::string> readOnlyKeys{UPTIME, CURRENT_TIME, FREE_MEMORY};
    inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, nvsDataKeys);

  std::string& getUptime();

  std::string& getCurrentTime();

  HasData &getData();
  std::string get(const std::string &key);

  coop_wifi::ConfigWithWiFi & getWifiConfig();

  void init();
  std::string & getFreeMemory();
}

#endif // COOP_TIME_H