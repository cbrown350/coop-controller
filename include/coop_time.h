#ifndef COOP_TIME_H
#define COOP_TIME_H

#include "HasData.h"
#include "coop_wifi.h"
#include "coop_settings.h"
#include "utils.h"

namespace coop_time {
/*
 * Data values:
 *  uptime
 *  current_time
 *  free_memory
 */

  std::string& getUptime();

  std::string& getCurrentTime();

  std::map<std::string, std::string> getData();
  std::string get(const std::string &key);

  coop_wifi::ConfigWithWiFi & getWifiConfig();

  void init();
  std::string & getFreeMemory();
}

#endif // COOP_TIME_H