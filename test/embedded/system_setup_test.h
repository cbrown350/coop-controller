#ifndef SYSTEM_SETUP_H_
#define SYSTEM_SETUP_H_

#include "settings.h"
#include <Arduino.h>
#include "wifi_test.h"

namespace system_setup_test {
    inline void setupWithWifi() {
        // must put Arduino init, Serial setup, and wifi setup before InitGoogleTest
        initArduino();
        Serial.begin(SERIAL_BAUD_RATE);
        wifi_test::connect();
    }
}  // namespace

#endif // SYSTEM_SETUP_H_