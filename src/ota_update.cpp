#ifndef OTA_UPDATE_CPP_
#define OTA_UPDATE_CPP_

#include "coop_settings.h"
#include <ArduinoOTA.h>
#include "ota_update.h"
#include "CoopLogger.h"
#include <thread>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils.h"

#include "SPIFFS.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include <condition_variable>

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

namespace ota_update {

    const char * const TAG = "ota_update";

    std::condition_variable devOTALoopCond;
    std::mutex devOTALoopMutex;
    std::thread *devOTALoopThread = nullptr;
    bool devOTALoopThreadStop = false;


    std::condition_variable serverOTALoopCond;
    std::mutex serverOTALoopMutex;
    // std::thread *serverOTALoopThread = nullptr;
    TaskHandle_t *serverOTALoopThread = nullptr;
    bool serverOTALoopThreadStop = false;

    
#ifdef SERVER_OTA_UPDATE_URL
    esp_err_t updateFromUrl(const char* url) { 
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE   
/* esp_http_client_config_t doesn't work if fully initialized, done per ESP-IDF docs */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        esp_http_client_config_t config = { 
            .url = url,
            .crt_bundle_attach = esp_crt_bundle_attach,
        };
#pragma GCC diagnostic pop
        CoopLogger::logi(TAG, "Starting OTA firmware update from URL: %s...", url);
        return esp_https_ota(&config);
#else
        return ESP_FAIL;
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    }

    void checkUrlForUpdate() {
        WiFiClientSecure client;
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE           
        client.setCACertBundle(x509_crt_imported_bundle_bin_start);
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE        
        HTTPClient http;
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        CoopLogger::logd(TAG, "SERVER_OTA_UPDATE_URL: %s", SERVER_OTA_UPDATE_URL);
        http.begin(client, SERVER_OTA_UPDATE_URL);
        http.addHeader("User-Agent", PRODUCT_NAME "/" VERSION_BUILD);
        http.addHeader("Accept", "application/json");
        int errorCode = http.GET();
        if (errorCode != HTTP_CODE_OK) {
            CoopLogger::loge(TAG, "HTTP GET failed, error: %s (%d)", http.errorToString(errorCode).c_str(), errorCode);
            return;
        }
        // String jsonFile = http.getString();
        // CoopLogger::logd(TAG, "jsonFile: %s", jsonFile.c_str());
        // http.end();

        // parson json file
        DynamicJsonDocument doc(1024);
        // DeserializationError error = deserializeJson(doc, jsonFile);
        DeserializationError error = deserializeJson(doc, http.getStream());
        http.end();
        if (error) {
            CoopLogger::loge(TAG, "deserializeJson() failed: %s", error.c_str());
            return;
        }
        const char* version = doc["version"];
        const char* url = doc["url"];
        CoopLogger::logd(TAG, "New firmware version: %s", version);
        CoopLogger::logd(TAG, "New firmware url: %s", url);

        // compare semantic using versioning var version with VERSION_BUILD
        if (strcmp(version, VERSION_BUILD) > 0) {
            CoopLogger::logi(TAG, "Current version: '%s' is older than server version: '%s', upgrading...", VERSION_BUILD, version);
            esp_err_t ret = updateFromUrl(url);
            if (ret == ESP_OK) {
                CoopLogger::logi(TAG, "Successfully upgraded firmware to version: %s", version);
            } else {
                CoopLogger::loge(TAG, "Firmware upgrade failed, error: %s", esp_err_to_name(ret));
            }
            std::this_thread::sleep_for(std::chrono::seconds(5));
            esp_restart();
        } else {
            CoopLogger::logd(TAG, "Current version: '%s' is not older than server version: '%s', skipping", VERSION_BUILD, version);
            return;
        }
        
    }  

    void serverOtaHandle(void * pvParameters) {
        if (WiFi.status() == WL_CONNECTED) 
            checkUrlForUpdate();
        while(utils::wait_for(std::chrono::seconds(SERVER_OTA_CHECK_INTERVAL_SECS), 
                    serverOTALoopMutex, serverOTALoopCond, serverOTALoopThreadStop)) {
            if (WiFi.status() == WL_CONNECTED) 
                checkUrlForUpdate();
        }
        vTaskDelete( NULL );
    }
#endif // SERVER_OTA_UPDATE_URL  

    void devOtaHandle() {        
        // while (WiFi.status() != WL_CONNECTED) {
        //     std::this_thread::sleep_for(std::chrono::seconds(5));
        // }
        // set up direct OTA
        ArduinoOTA.setHostname(HOSTNAME);
        ArduinoOTA.setPassword(DEV_OTA_UPDATE_PASSWORD);
        static std::string type;
        ArduinoOTA
            .onStart([]() {
                if (ArduinoOTA.getCommand() == U_FLASH) {
                    type = "firmware";
                } else {// U_SPIFFS
                    type = "filesystem";
                    SPIFFS.end();
                }                
                CoopLogger::logi(TAG, "Start updating %s", type.c_str());
                })
            .onEnd([]() {
                CoopLogger::logi(TAG, "%s OTA update finished", type.c_str());
                if(type.compare("filesystem") == 0) 
                    SPIFFS.begin();
                })
            .onProgress([](unsigned int progress, unsigned int total) {
                CoopLogger::getDefaultPrintStream()->printf("%s update progress: %u%%           \r", type.c_str(), (progress / (total / 100)));
                })
            .onError([](ota_error_t error) {
                CoopLogger::loge(TAG, "Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) CoopLogger::loge(TAG, "Auth Failed");
                else if (error == OTA_BEGIN_ERROR) CoopLogger::loge(TAG, "Begin Failed");
                else if (error == OTA_CONNECT_ERROR) CoopLogger::loge(TAG, "Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) CoopLogger::loge(TAG, "Receive Failed");
                else if (error == OTA_END_ERROR) CoopLogger::loge(TAG, "End Failed");
                });
        ArduinoOTA.begin();
        while(utils::wait_for(std::chrono::seconds(1), devOTALoopMutex, devOTALoopCond, devOTALoopThreadStop)) {
            if (WiFi.status() == WL_CONNECTED) 
                ArduinoOTA.handle();
        }
    }

    void init() {
        devOTALoopThreadStop = false;
        devOTALoopThread = new std::thread(devOtaHandle);

#ifdef SERVER_OTA_UPDATE_URL
        serverOTALoopThreadStop = false;
        // serverOTALoopThread = new std::thread(devOtaHandle);
        // using low-level calls for more stack instead of std::thread
        xTaskCreate(serverOtaHandle,          /* Task function. */
                "serverOtaHandle",        /* String with name of task. */
                10000,            /* Stack size in bytes. */
                NULL,             /* Parameter passed as input of the task */
                1,                /* Priority of the task. */
                serverOTALoopThread);
#endif // SERVER_OTA_UPDATE_URL                
    }

    void deinit() {
        if(devOTALoopThread) {
            {
                std::scoped_lock<std::mutex> l(devOTALoopMutex);
                devOTALoopThreadStop = true;
            }
            devOTALoopCond.notify_one();
            devOTALoopThread->join();
            delete devOTALoopThread;
            devOTALoopThread = nullptr;
        }

        if(serverOTALoopThread) {
            {
                std::scoped_lock<std::mutex> l(serverOTALoopMutex);
                serverOTALoopThreadStop = true;
            }
            serverOTALoopCond.notify_one();
        //     serverOTALoopThread->join();
        //     delete serverOTALoopThread;
            while(eTaskGetState(serverOTALoopThread) != eDeleted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            serverOTALoopThread = nullptr;
        }
    }
} // namespace ota_update


#endif // OTA_UPDATE_CPP_