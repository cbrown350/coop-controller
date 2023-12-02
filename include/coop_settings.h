#ifndef COOP_SETTINGS_H_
#define COOP_SETTINGS_H_

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


#define SERIAL_BAUD_RATE                        115200 // 921600 //

// Vars below can be set via a .env file at the project root
#ifndef PRODUCT_NAME
#define PRODUCT_NAME                             "Coop Controller"
#endif

#ifndef HOSTNAME
#define HOSTNAME                                "CoopController" 
#endif

#ifndef COOP_CONTROLLER_AP_NAME
#define COOP_CONTROLLER_AP_NAME                 PRODUCT_NAME
#endif

#ifndef COOP_CONTROLLER_AP_PASSWORD
#define COOP_CONTROLLER_AP_PASSWORD             NULL
#pragma warning("COOP_CONTROLLER_AP_PASSWORD not defined, the wifi setup AP will not be password protected.")
#endif

#ifndef FACTORY_RESET_TIME_SECS                 // time to press the reset button for a factory reset
#define FACTORY_RESET_TIME_SECS                 10
#endif

#ifndef WIFI_SETUP_TIMEOUT_SECS                 // time to wait for wifi AP setup until it restarts
#define WIFI_SETUP_TIMEOUT_SECS                 300
#endif

#ifndef SERVER_OTA_UPDATE_URL
#pragma warning("SERVER_OTA_UPDATE_URL not defined, not OTA updates from a server will be available.")
#endif

#ifndef SERVER_OTA_CHECK_INTERVAL_SECS
#define SERVER_OTA_CHECK_INTERVAL_SECS          3600
#endif

#ifndef DEV_OTA_UPDATE_PASSWORD
#define DEV_OTA_UPDATE_PASSWORD                 NULL
#pragma warning("DEV_OTA_UPDATE_PASSWORD not defined, OTA updates can be performed without any password protection")
#endif

#ifndef DEFAULT_MQTT_SERVER
#define DEFAULT_MQTT_SERVER                     "mosquitto"
#endif

#ifndef DEFAULT_MQTT_PORT
#define DEFAULT_MQTT_PORT                       "1883"
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER                       "mosquitto"
#endif

#ifndef DEFAULT_MQTT_PASSWORD
#define DEFAULT_MQTT_PASSWORD                   "password"
#endif

// #define SYSLOG_SERVER                           "syslog-server"

#ifndef SYSLOG_PORT
#define SYSLOG_PORT                             514
#endif

// #define ENABLE_LOGGING

// end .env settings


#ifndef FIRMWARE_VERSION                           
#pragma error("You must set the firmware version in the build system.")
#endif

#define VERSION_BUILD                           FIRMWARE_VERSION  "+" TOSTRING(BUILD_NUM)
#define PRODUCT_VERSION                         PRODUCT_NAME " " VERSION_BUILD

#ifdef IS_DEBUG_BUILD
#ifndef ENABLE_LOGGING
#define ENABLE_LOGGING
#endif
#endif

#include <Arduino.h>

#define RED_LED_OUT_B                           GPIO_NUM_3
#define GREEN_LED_OUT_B                         GPIO_NUM_26 // DAC_CHANNEL_2_GPIO_NUM
#define DOOR_IS_CLOSED_IN_B                     GPIO_NUM_2
#define DOOR_IS_OPEN_IN_B                       GPIO_NUM_17
#define DOOR_OPEN_OUT                           GPIO_NUM_4
#define DOOR_CLOSE_OUT                          GPIO_NUM_5
#define WATER_LEVEL_LOW_IN                      GPIO_NUM_16
#define BUZZER_OUT                              GPIO_NUM_25 // DAC_CHANNEL_1_GPIO_NUM
#define WATER_HEATER_EN_OUT_B                   GPIO_NUM_18
#define WATER_METER_PULSE_IN                    GPIO_NUM_19
#define OLED_I2C_SDA                            GPIO_NUM_21
#define OLED_I2C_SCL                            GPIO_NUM_22
#define SPOTLIGHT_EN_OUT_B                      GPIO_NUM_23
#define ZERO_CROSS_IN_B                         GPIO_NUM_27
#define BOX_DOOR_IS_OPEN_ADC_IN                 GPIO_NUM_32
#define OUTSIDE_TEMP_ADC_IN                     GPIO_NUM_34
#define COOP_TEMP_ADC_IN                        GPIO_NUM_33
#define WATER_TEMP_ADC_IN                       GPIO_NUM_35
#define DOOR_IS_CLOSING_OVERLOAD_ADC_IN         GPIO_NUM_39
#define FOOD_NOT_EMPTY_ADC_IN                   GPIO_NUM_36
#define FACTORY_RESET_BUTTON_IN_B               GPIO_NUM_0


#endif // COOP_SETTINGS_H_