#include "factory_reset.h"  

#include "coop_settings.h"
#include "CoopLogger.h"

#include <thread>
#include <vector>

namespace factory_reset {
  using std::thread;
  using std::function;
  using std::vector;

  const char * const TAG = "FactoryReset";
    
  bool factory_reset = false;
  std::mutex addResetCallbackMutex;
  vector<function<void()>> onFactoryResetCallbacks = {};

  void IRAM_ATTR factoryResetIsr() {
    if(digitalRead(FACTORY_RESET_BUTTON_IN_B) == LOW)
      factory_reset = true;
    else
      factory_reset = false;
  }

  void factoryResetLoop() {
    while(true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      if(factory_reset) {
        CoopLogger::logi(TAG, "Starting factory reset process, will continue if button held for %d seconds...", FACTORY_RESET_TIME_SECS);
        std::this_thread::sleep_for(std::chrono::milliseconds(FACTORY_RESET_TIME_SECS * 1000));
        if(factory_reset) {
          CoopLogger::logi(TAG, "Factory reset triggered...");
          for (const auto &onFactoryResetCallback : onFactoryResetCallbacks)
            onFactoryResetCallback();
          CoopLogger::logi(TAG, "Factory reset callbacks have been run, restarting...");
          std::this_thread::sleep_for(std::chrono::milliseconds(5000));
          ESP.restart();
        } else {
          CoopLogger::logi(TAG, "Factory reset cancelled");
        }
      }
    }
  }

  void init() {
    CoopLogger::logi(TAG, "Setting up factory reset on pin %d", FACTORY_RESET_BUTTON_IN_B);
    pinMode(FACTORY_RESET_BUTTON_IN_B, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(FACTORY_RESET_BUTTON_IN_B), factoryResetIsr, CHANGE);
    interrupts();

    thread t(factoryResetLoop);
    t.detach(); //NOSONAR - won't fix, intended to run indefinitely
  }

  void addOnFactoryResetCallback(const function<void()> &onFactoryResetCallback) {
    CoopLogger::logi(TAG, "Adding on factory reset callback");
    std::scoped_lock<std::mutex> lock(addResetCallbackMutex);
    factory_reset::onFactoryResetCallbacks.push_back(onFactoryResetCallback);
  }
}