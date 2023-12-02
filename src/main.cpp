#include <Arduino.h>
#include "sdkconfig_local.h"

#include "ac_zero_crossing.h"
#include "BuzzerController.h"
#include "coop_settings.h"
#include "coop_time.h"
#include "coop_wifi.h"
#include "Dimmer.h"
#include "DoorController.h"
#include "ElogCoopLogger.h"
#include "SyslogCoopLogger.h"
#include "factory_reset.h"
#include "HeaterController.h"
#include "LEDController.h"
#include "LeadingEdgePhaseDimmer.h"
#include "LightSensor.h"
#include "MQTTController.h"
#include "OLEDDisplayController.h"
#include "ota_update.h"
#include "PulseSkipModulationDimmer.h"
#include "sunrise_sunset.h"
#include "TemperatureSensor.h"
#include "WaterFlowMeter.h"
#include "WebServer.h"

#include <memory>

void test() {
  CoopLogger::logi("testasdfasfdsadfdd", "here %s", "[some string]");
}



extern "C" void app_main() {
  static MQTTController mqtt;
  static DoorController door;
  bool test1 = door.getData().door_open_sensed;


  initArduino();

  esp_log_level_set("*", ESP_LOG_ERROR);
  esp_log_level_set("wifi", ESP_LOG_WARN);
  CoopLogger::addLogger(std::make_unique<ElogCoopLogger>(Serial));
  CoopLogger::addLogger(std::make_unique<SyslogCoopLogger>());
  CoopLogger::setDefaultPrintStream(&Serial);
  CoopLogger::logi("Main", "Starting %s version %s",  PRODUCT_NAME, VERSION_BUILD);

  factory_reset::addOnFactoryResetCallback(coop_wifi::resetWifi);
  factory_reset::init();

  coop_wifi::addSetupParams(mqtt.getSettingParams());
  coop_wifi::init(*CoopLogger::getDefaultPrintStream(), {test});

  ota_update::init();

  mqtt.init();
}