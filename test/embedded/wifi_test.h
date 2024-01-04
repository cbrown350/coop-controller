#ifndef TEST_WIFI_CPP
#define TEST_WIFI_CPP

#include "settings.h"
#include <WiFi.h>
#include <HardwareSerial.h>

namespace wifi_test {
    inline void connect() {
        Serial.printf("Connecting to %s...\n", TEST_SSID);
        WiFiGenericClass::mode(WIFI_STA);
        WiFi.begin(TEST_SSID, TEST_PASSWORD);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        Serial.println("\nWiFi connected");
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(WiFi.SSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
} // namespace

#endif // TEST_WIFI_CPP
