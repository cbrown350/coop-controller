#if defined(PIO_UNIT_TESTING) && not defined(ARDUINO)
int main(int argc, char **argv) { }
#else

#include <Arduino.h>
#include "sdkconfig_local.h"
#include "settings.h"

#include <ZeroCrossing.h>
#include <BuzzerController.h>
#include <ntp_time.h>
#include <cwifi.h>
#include <DoorController.h>
#include <SimpleLogger.h>
#include <SyslogLogger.h>
#include <factory_reset.h>
#include <HeaterController.h>
#include <Heartbeat.h>
#include <LeadingEdgePhaseDimmer.h>
#include <LightSensor.h>
#include "MQTTController.h"
#include <OLEDDisplayPrinter.h>
#include <ota_update.h>
#include <PulseSkipModulationDimmer.h>
#include <sunrise_sunset.h>
#include <TemperatureSensor.h>
#include <WaterFlowMeter.h>
#include <DashboardServer.h>

#include <memory>

static const char * const TAG = "Main";

extern "C" void app_main() {
    // Arduino
  initArduino();

  // Logging
//  Logger<>::setTagLevel(TAG, LOG_LEVEL_INFO);
  Logger<>::addLogger(std::make_unique<SimpleLogger>(true, Serial, LOG_LEVEL_INFO));
  Logger<>::setTagLevel(SyslogLogger::TAG, LOG_LEVEL_INFO);
  Logger<>::addLogger(std::make_unique<SyslogLogger>());
  Logger<>::logi(TAG, "Starting %s version %s",  PRODUCT_NAME, VERSION_BUILD);


  Logger<>::setTagLevel(cwifi::TAG, LOG_LEVEL_INFO);

  // Factory reset
  Logger<>::setTagLevel(factory_reset::TAG, LOG_LEVEL_INFO);
  factory_reset::init();

  // OTA config
  Logger<>::setTagLevel(ota_update::TAG, LOG_LEVEL_INFO);
  cwifi::addOnIPAddressCallback([]() { ota_update::startLoop(true); });
  cwifi::addOnDisconnectedCallback(ota_update::stopLoop);

  // MQTT config
//  Logger<>::setTagLevel(MQTTController::TAG, LOG_LEVEL_INFO);
  static MQTTController mqtt{"mqtt"};
  mqtt.init();
  mqtt.setLastWill(MQTT_TOPIC_PREFIX "/status");
  factory_reset::addOnFactoryResetCallback([]() { [[maybe_unused]] auto no_use = mqtt.deleteNvsData(); });
  cwifi::addConfigWithWiFi(&mqtt);
  cwifi::addOnIPAddressCallback([]() { mqtt.startLoop(); });
  cwifi::addOnDisconnectedCallback([]() { mqtt.stopLoop(); });

  // Time
  Logger<>::setTagLevel(ntp_time::TAG, LOG_LEVEL_INFO);
  ntp_time::init(); 
  factory_reset::addOnFactoryResetCallback([]() { [[maybe_unused]] auto no_use = ntp_time::getData().deleteNvsData(); });
  cwifi::addConfigWithWiFi(&ntp_time::getWifiConfig());
  cwifi::addOnIPAddressCallback([]() { ntp_time::update(); });
  mqtt.registerHasDataItem(&ntp_time::getData());

  // WiFi
  factory_reset::addOnFactoryResetCallback(cwifi::resetWifi);
  mqtt.registerHasDataItem(&cwifi::getData());
  cwifi::init(); // TODO: check interaction with timer in LeadingEdgePhaseDimmer

  // Coop Run Light
  Logger<>::setTagLevel(LeadingEdgePhaseDimmer::TAG, LOG_LEVEL_INFO);
  static LeadingEdgePhaseDimmer runLightDimmer{"run_light", RUN_LIGHT_EN_OUT_B};
  mqtt.registerHasDataItem(&runLightDimmer);
  runLightDimmer.setBrightness(100, LeadingEdgePhaseDimmer::RESOLUTION_MAX_SIZE, 00);

  // Coop Roost Door
//  static DoorController door{"door", DOOR_IS_OPEN_IN_B, DOOR_IS_CLOSED_IN_B, DOOR_OPEN_OUT, DOOR_CLOSE_OUT, DOOR_IS_CLOSING_OVERLOAD_ADC_IN};



 std::thread testlightThread([]() {
  //  runLightDimmer.setBrightness(0, LeadingEdgePhaseDimmer::RESOLUTION_MAX_SIZE);
   while(true) {
        for(int i = runLightDimmer.getMinNoFlickerBrightness(); i < runLightDimmer.getResolution(); i+=1) {
            runLightDimmer.setBrightness(i);
            vTaskDelay(1 / portTICK_PERIOD_MS);
//            if(i % 1000 == 0)
//                Logger<>::logi(TAG, "brightness: %d, frequency: %s", i, runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
        }
        for(int i = runLightDimmer.getResolution(); i >= runLightDimmer.getMinNoFlickerBrightness(); i-=1) {
            runLightDimmer.setBrightness(i);
            vTaskDelay(1 / portTICK_PERIOD_MS);
//            if(i % 1000 == 0)
//                Logger<>::logi(TAG, "brightness: %d, frequency: %s", i, runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
        }
       runLightDimmer.setBrightness(0);
       vTaskDelay(1 / portTICK_PERIOD_MS);
          //  vTaskDelay(10 / portTICK_PERIOD_MS);

//        auto freq1 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
//        unsigned tmpHalfPeriodUs1 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
//        Logger<>::logi(TAG, "freq: %s, period/2: %d", freq1.c_str(), tmpHalfPeriodUs1);
//        for(int i = runLightDimmer.getResolution()-2; i >= 1; i--) {
//            runLightDimmer.setBrightness(i);
//            vTaskDelay(1 / portTICK_PERIOD_MS);
//            if(i % 1000 == 0)
//                Logger<>::logi(TAG, "brightness: %d, frequency: %s", i, runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
//        }

//        auto freq2 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
//        unsigned tmpHalfPeriodUs2 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
//        Logger<>::logi(TAG, "freq: %s, period/2: %d", freq2.c_str(), tmpHalfPeriodUs2);
//        if(std::abs((int)tmpHalfPeriodUs1 - (int)tmpHalfPeriodUs2) > LeadingEdgePhaseDimmer::TIMER_TICK_US * 2)
//                Logger<>::logw(TAG, "halfPeriod_us changed from %d to %d, may be excessive jitter (%sHz <-> %sHz)",
//                                 tmpHalfPeriodUs1, tmpHalfPeriodUs2, freq1.c_str(), freq2.c_str());

//        runLightDimmer.setBrightness(800);
//        Logger<>::logi(TAG, "waiting 5s at brightness %d", runLightDimmer.getBrightness());
//        vTaskDelay(5000 / portTICK_PERIOD_MS);


//    }
//     runLightDimmer.setBrightness(0, LeadingEdgePhaseDimmer::RESOLUTION_MAX_SIZE, 800);
//     while(true) {
//         constexpr int maxBrightness = 2048;
//         for(int i = 0; i < maxBrightness; i+=1) {
//             runLightDimmer.setBrightness(i);
//             vTaskDelay(1 / portTICK_PERIOD_MS);
//             if(i % 500 == 0) {
//                 Logger<>::logi(TAG, "brightness: %d, frequency: %s", i,
//                                  runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
//             }
//         }
// //        Logger<>::logi(TAG, "waiting 5s at brightness %d", runLightDimmer.getBrightness());
// //        vTaskDelay(5000 / portTICK_PERIOD_MS);

//         auto freq1 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
//         unsigned tmpHalfPeriodUs1 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
//         Logger<>::logi(TAG, "freq: %s, period/2: %d", freq1.c_str(), tmpHalfPeriodUs1);
//         for(int i = maxBrightness; i >= 1; i-=1) {
//             runLightDimmer.setBrightness(i);
//             vTaskDelay(1 / portTICK_PERIOD_MS);
//             if(i % 500 == 0) {
//                 Logger<>::logi(TAG, "brightness: %d, frequency: %s", i,
//                                  runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
//             }
//         }

//         auto freq2 = runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY);
//         unsigned tmpHalfPeriodUs2 = LeadingEdgePhaseDimmer::getHalfPeriodUs();
//         Logger<>::logi(TAG, "freq: %s, period/2: %d", freq2.c_str(), tmpHalfPeriodUs2);
//         if(std::abs((int)tmpHalfPeriodUs1 - (int)tmpHalfPeriodUs2) > LeadingEdgePhaseDimmer::TIMER_TICK_US * 2)
//             Logger<>::logw(TAG, "halfPeriod_us changed from %d to %d, may be excessive jitter (%sHz <-> %sHz)",
//                              tmpHalfPeriodUs1, tmpHalfPeriodUs2, freq1.c_str(), freq2.c_str());

//         runLightDimmer.setBrightness(10);
//         Logger<>::logi(TAG, "waiting 5s at brightness %d", runLightDimmer.getBrightness());
//         Logger<>::logi(TAG, "frequency: %s",
//                              runLightDimmer.get(LeadingEdgePhaseDimmer::FREQUENCY).c_str());
//         vTaskDelay(5000 / portTICK_PERIOD_MS);

    }
 });
 testlightThread.detach();
  //  while(true) {
  //      runLightDimmer.setBrightness(runLightDimmer.getResolution() / 2);
  //      Logger<>::logw(TAG, "half brightness: %d", runLightDimmer.getBrightness());
  //      vTaskDelay(2000 / portTICK_PERIOD_MS);
  //      runLightDimmer.setBrightness(runLightDimmer.getResolution() - 1);
  //      Logger<>::logw(TAG, "full: %d", runLightDimmer.getBrightness());
  //      vTaskDelay(2000 / portTICK_PERIOD_MS);
  //      runLightDimmer.setBrightness(0);
  //      Logger<>::logw(TAG, "off brightness: %d", runLightDimmer.getBrightness());
  //      vTaskDelay(2000 / portTICK_PERIOD_MS);
  //  }

// //  bool test1 = door.getBool("door_open_sensed");

}


#endif // PIO_UNIT_TESTING