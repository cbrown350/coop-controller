#include "ota_update.h"
#include "settings.h"

#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPIFFS.h>
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_crt_bundle.h>
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

#include <Logger.h>
#include <utils.h>

#include <thread>
#include <condition_variable>

#include <ArduinoJson.h>

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
//extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

namespace ota_update {

    inline static constexpr const char * TAG{"otaup"};

    static bool _restartOnUpdate = true;

    std::condition_variable devOTALoopCond;
    std::mutex devOTALoopMutex;
    std::thread *devOTALoopThread = nullptr;
    bool devOTALoopThreadStop = true;


    std::condition_variable serverOTALoopCond;
    std::mutex serverOTALoopMutex;
    // std::thread *serverOTALoopThread = nullptr;
    BaseType_t serverOTALoopThread;
    TaskHandle_t serverOTALoopThreadHandle = nullptr;
    bool serverOTALoopThreadStop = true;

    void updateCompleted() {
        if(_restartOnUpdate) {
            Logger::logi(TAG, "Restarting in 5 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            esp_restart();
        }
    }
    
#ifdef SERVER_OTA_UPDATE_URL
    esp_err_t updateFromUrl(const char* url) {
        Logger::logv(TAG, "[updateFromUrl]");
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE   
/* esp_http_client_config_t doesn't work if fully initialized, done per ESP-IDF docs */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
        arduino_esp_crt_bundle_set(x509_crt_imported_bundle_bin_start);
        esp_http_client_config_t config = {
            .url = url,
            // .crt_bundle_attach = esp_crt_bundle_attach,
            .crt_bundle_attach = arduino_esp_crt_bundle_attach,
        };
#pragma GCC diagnostic pop
        Logger::logi(TAG, "Starting OTA firmware update from URL: %s...", url);
        return esp_https_ota(&config);
#else
        return ESP_FAIL;
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    }

    void checkUrlForUpdate() {
        Logger::logv(TAG, "[checkUrlForUpdate]");
        WiFiClientSecure client;
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE           
        client.setCACertBundle(x509_crt_imported_bundle_bin_start);
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE        
        HTTPClient http;
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        Logger::logd(TAG, "SERVER_OTA_UPDATE_URL: %s", SERVER_OTA_UPDATE_URL);
        http.begin(client, SERVER_OTA_UPDATE_URL);
        http.addHeader("User-Agent", PRODUCT_NAME "/" VERSION_BUILD);
        http.addHeader("Accept", "application/json");
        int errorCode = http.GET();
        if (errorCode != HTTP_CODE_OK) {
            Logger::loge(TAG, "HTTP GET failed, error: %s (%d)", HTTPClient::errorToString(errorCode).c_str(), errorCode);
            return;
        }
        // String jsonFile = http.getString();
        // Logger::logd(TAG, "jsonFile: %s", jsonFile.c_str());
        // http.end();

        // parson json file
        DynamicJsonDocument doc(1024);
        // DeserializationError error = deserializeJson(doc, jsonFile);
        DeserializationError error = deserializeJson(doc, http.getStream());
        http.end();
        if (error) {
            Logger::loge(TAG, "deserializeJson() failed: %s", error.c_str());
            return;
        }
        const char* version = doc["version"];
        const char* url = doc["url"];
        Logger::logd(TAG, "New firmware version: %s", version);
        Logger::logd(TAG, "New firmware url: %s", url);

        // compare semantic using versioning var version with VERSION_BUILD
        if (strcmp(version, VERSION_BUILD) > 0) {
            Logger::logi(TAG, "Current version: '%s' is older than server version: '%s', upgrading...", VERSION_BUILD, version);
            esp_err_t ret = updateFromUrl(url);
            if (ret == ESP_OK) {
                Logger::logi(TAG, "Successfully upgraded firmware to version: %s from the server", version);
            } else {
                Logger::loge(TAG, "Firmware upgrade failed, error: %s", esp_err_to_name(ret));
            }
            updateCompleted();
        } else {
            Logger::logd(TAG, "Current version: '%s' is not older than server version: '%s', skipping", VERSION_BUILD, version);
        }
        
    }  

    void serverOtaHandle(void *) {
        Logger::logv(TAG, "[serverOtaHandle]");
        do {
            if (WiFiSTAClass::status() == WL_CONNECTED)
                checkUrlForUpdate();
        } while(utils::wait_for(std::chrono::seconds(SERVER_OTA_CHECK_INTERVAL_SECS),
                              serverOTALoopMutex, serverOTALoopCond, serverOTALoopThreadStop));

        Logger::logv(TAG, "[serverOtaHandle] terminating...");
        vTaskDelete( serverOTALoopThreadHandle );
    }
#endif // SERVER_OTA_UPDATE_URL  

    void devOtaHandle() {
        Logger::logv(TAG, "[devOtaHandle]");
        
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
                Logger::logi(TAG, "Start updating %s", type.c_str());
                })
            .onEnd([]() {
                Logger::logi(TAG, "%s OTA update finished", type.c_str());
                if (type == "filesystem")
                    SPIFFS.begin();

                updateCompleted();
            })
            .onProgress([](unsigned progress, unsigned total) {
                Logger::getDefaultPrintStream()->printf("%s update progress: %u%%           \r", type.c_str(), (progress / (total / 100)));
                })
            .onError([](ota_error_t error) {
                Logger::loge(TAG, "Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Logger::loge(TAG, "Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Logger::loge(TAG, "Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Logger::loge(TAG, "Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Logger::loge(TAG, "Receive Failed");
                else if (error == OTA_END_ERROR) Logger::loge(TAG, "End Failed");
                });
        ArduinoOTA.begin();
        Logger::logi(TAG, "OTA update accepting connections");
        while(utils::wait_for(std::chrono::seconds(1), devOTALoopMutex, devOTALoopCond, devOTALoopThreadStop)) {
            if (WiFiSTAClass::status() == WL_CONNECTED)
                ArduinoOTA.handle();
        }
        Logger::logv(TAG, "[devOtaHandle] terminating...");
    }

    void startLoop(const bool restartOnUpdate) {
        Logger::logv(TAG, "[startLoop]");
        _restartOnUpdate = restartOnUpdate;
        {
            std::scoped_lock<std::mutex> l{devOTALoopMutex};
            devOTALoopThreadStop = false;
            devOTALoopThread = new std::thread(devOtaHandle);
        }

#ifdef SERVER_OTA_UPDATE_URL
        {
            std::scoped_lock<std::mutex> l{serverOTALoopMutex};
            serverOTALoopThreadStop = false;
            // serverOTALoopThread = new std::thread(devOtaHandle);
            // using low-level calls for more stack instead of std::thread
            serverOTALoopThread = xTaskCreate(serverOtaHandle,          /* Task function. */
                                              "serverOtaHandle",        /* String with name of task. */
                                              10000,            /* Stack size in bytes. */
                                              nullptr,             /* Parameter passed as input of the task */
                                              1,                /* Priority of the task. */
                                              &serverOTALoopThreadHandle);
        }
#endif // SERVER_OTA_UPDATE_URL                
    }

    void stopLoop() {
        Logger::logv(TAG, "[stopLoop]");

        if(!devOTALoopThreadStop) {
            Logger::logv(TAG, "devOTALoopThread sending stop signal...");
            {
                std::scoped_lock<std::mutex> l{devOTALoopMutex};
                devOTALoopThreadStop = true;
            }
            devOTALoopCond.notify_all();
            devOTALoopThread->join();
            delete devOTALoopThread;
            devOTALoopThread = nullptr;
        }
        Logger::logv(TAG, "devOTALoopMutex stopped");

        if(serverOTALoopThread == pdPASS && !serverOTALoopThreadStop) {
            Logger::logv(TAG, "serverOTALoopThread sending stop signal...");
            {
                std::scoped_lock<std::mutex> l{serverOTALoopMutex};
                serverOTALoopThreadStop = true;
            }
            serverOTALoopCond.notify_all();
        //     serverOTALoopThread->join();
        //     delete serverOTALoopThread;
            while(eTaskGetState(serverOTALoopThreadHandle) != eDeleted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            serverOTALoopThreadHandle = NULL;
            serverOTALoopThread = 0; 
        }        
        Logger::logv(TAG, "serverOTALoopThread stopped");
    }
} // namespace ota_update
