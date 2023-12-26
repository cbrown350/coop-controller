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


extern "C" void app_main() {
    // Arduino
  initArduino();

    // static objs to keep after app_main returns
  static MQTTController mqtt{"mqtt"};
  static LeadingEdgePhaseDimmer runLightDimmer{"run_light", RUN_LIGHT_EN_OUT_B};
//    static DoorController door{"door", DOOR_IS_OPEN_IN_B, DOOR_IS_CLOSED_IN_B, DOOR_OPEN_OUT, DOOR_CLOSE_OUT, DOOR_IS_CLOSING_OVERLOAD_ADC_IN};

//
  // Logging
#if defined(ENABLE_LOGGING) && defined(IS_DEBUG_BUILD) // only enable serial port output on debug
  CoopLogger::addLogger(std::make_unique<ElogCoopLogger>(Serial));
#endif
  CoopLogger::addLogger(std::make_unique<SyslogCoopLogger>());
  CoopLogger::setDefaultPrintStream(&Serial);
  CoopLogger::logi("Main", "Starting %s version %s",  PRODUCT_NAME, VERSION_BUILD);

  // Factory reset
  factory_reset::addOnFactoryResetCallback(coop_wifi::resetWifi);
  factory_reset::addOnFactoryResetCallback([]() { mqtt.deleteNvsData(); });
  factory_reset::addOnFactoryResetCallback([]() { coop_time::getData().deleteNvsData(); });
  factory_reset::init();

  // Time
  coop_time::init();
  coop_time::getData().set(coop_time::TIMEZONE, "America/New_York");

  // Wifi - OTA config
  coop_wifi::addOnIPAddressCallback(ota_update::startLoop);
  coop_wifi::addOnDisconnectedCallback(ota_update::stopLoop);

  // Wifi - MQTT config
  mqtt.init();
  coop_wifi::addConfigWithWiFi(&mqtt);
  coop_wifi::addOnIPAddressCallback([]() { mqtt.startLoop(); });
  coop_wifi::addOnDisconnectedCallback([]() { mqtt.stopLoop(); });
  mqtt.setLastWill("coop/status");
  mqtt.registerHasDataItem(&runLightDimmer);

  // Wifi - time config
  coop_wifi::addConfigWithWiFi(&coop_time::getWifiConfig());

  coop_wifi::init();



//  std::thread testlightThread([]() {
      pinMode(GREEN_LED_OUT_B, OUTPUT);
      pinMode(RED_LED_OUT_B, OUTPUT);
//    runLightDimmer.setBrightness(0, LeadingEdgePhaseDimmer::RESOLUTION_MAX_SIZE);
//    while(true) {
//        for(int i = 0; i < runLightDimmer.getResolution(); i+=2) {
//            runLightDimmer.setBrightness(i);
//            vTaskDelay(1 / portTICK_PERIOD_MS);
//            if(i % 1000 == 0)
//                CoopLogger::logi("Main", "brightness: %d, frequency: %s", i, runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
//        }
//
//        auto freq1 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
//        unsigned tmpHalfPeriodUs1 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
//        CoopLogger::logi("Main", "freq: %s, period/2: %d", freq1.c_str(), tmpHalfPeriodUs1);
//        for(int i = runLightDimmer.getResolution()-2; i >= 1; i--) {
//            runLightDimmer.setBrightness(i);
//            vTaskDelay(1 / portTICK_PERIOD_MS);
//            if(i % 1000 == 0)
//                CoopLogger::logi("Main", "brightness: %d, frequency: %s", i, runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
//        }
//
//        auto freq2 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
//        unsigned tmpHalfPeriodUs2 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
//        CoopLogger::logi("Main", "freq: %s, period/2: %d", freq2.c_str(), tmpHalfPeriodUs2);
//        if(std::abs((int)tmpHalfPeriodUs1 - (int)tmpHalfPeriodUs2) > LeadingEdgePhaseDimmer::TIMER_TICK_US * 2)
//                CoopLogger::logw("Main", "halfPeriod_us changed from %d to %d, may be excessive jitter (%sHz <-> %sHz)",
//                                 tmpHalfPeriodUs1, tmpHalfPeriodUs2, freq1.c_str(), freq2.c_str());
//
//        runLightDimmer.setBrightness(800);
//        CoopLogger::logi("Main", "waiting 5s at brightness %d", runLightDimmer.getBrightness());
//        vTaskDelay(5000 / portTICK_PERIOD_MS);
//
//
//    }
    runLightDimmer.setBrightness(0, LeadingEdgePhaseDimmer::RESOLUTION_MAX_SIZE, 800);
    while(true) {
        constexpr int maxBrightness = 2048;
        for(int i = 0; i < maxBrightness; i+=1) {
            runLightDimmer.setBrightness(i);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            if(i % 500 == 0) {
                CoopLogger::logi("Main", "brightness: %d, frequency: %s, zeroXPulseWidth us: %d", i,
                                 runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str(), LeadingEdgePhaseDimmer::getZeroXHalfPulseWidthUs());
            }
        }
//        CoopLogger::logi("Main", "waiting 5s at brightness %d", runLightDimmer.getBrightness());
//        vTaskDelay(5000 / portTICK_PERIOD_MS);

        auto freq1 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
        unsigned tmpHalfPeriodUs1 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
        CoopLogger::logi("Main", "freq: %s, period/2: %d", freq1.c_str(), tmpHalfPeriodUs1);
        for(int i = maxBrightness; i >= 1; i-=1) {
            runLightDimmer.setBrightness(i);
            vTaskDelay(1 / portTICK_PERIOD_MS);
            if(i % 500 == 0) {
                CoopLogger::logi("Main", "brightness: %d, frequency: %s, zeroXPulseWidth us: %d", i,
                                 runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str(), LeadingEdgePhaseDimmer::getZeroXHalfPulseWidthUs());
            }
        }

        auto freq2 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
        unsigned tmpHalfPeriodUs2 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
        CoopLogger::logi("Main", "freq: %s, period/2: %d", freq2.c_str(), tmpHalfPeriodUs2);
        if(std::abs((int)tmpHalfPeriodUs1 - (int)tmpHalfPeriodUs2) > LeadingEdgePhaseDimmer::TIMER_TICK_US * 2)
            CoopLogger::logw("Main", "halfPeriod_us changed from %d to %d, may be excessive jitter (%sHz <-> %sHz)",
                             tmpHalfPeriodUs1, tmpHalfPeriodUs2, freq1.c_str(), freq2.c_str());

        runLightDimmer.setBrightness(10);
        CoopLogger::logi("Main", "waiting 5s at brightness %d", runLightDimmer.getBrightness());
        CoopLogger::logi("Main", "frequency: %s, zeroXPulseWidth us: %d",
                             runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str(), LeadingEdgePhaseDimmer::getZeroXHalfPulseWidthUs());
        vTaskDelay(5000 / portTICK_PERIOD_MS);


    }
//  });
//  testlightThread.detach();
//    while(true) {
//        runLightDimmer.setBrightness(runLightDimmer.getResolution() / 2);
//        CoopLogger::logw("Main", "half brightness: %d", runLightDimmer.getBrightness());
//        vTaskDelay(10000 / portTICK_PERIOD_MS);
//        runLightDimmer.setBrightness(runLightDimmer.getResolution() - 1);
//        CoopLogger::logw("Main", "full: %d", runLightDimmer.getBrightness());
//        vTaskDelay(10000 / portTICK_PERIOD_MS);
//        runLightDimmer.setBrightness(0);
//        CoopLogger::logw("Main", "off brightness: %d", runLightDimmer.getBrightness());
//        vTaskDelay(10000 / portTICK_PERIOD_MS);
//    }

//  bool test1 = door.getBool("door_open_sensed");
}