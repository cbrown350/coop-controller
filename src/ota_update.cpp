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

#include <Update.h>

// #include <mbedtls/config.h>
// #define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED

// #include "esp_wifi.h"
// #include "esp_event.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "nvs_flash.h"
// #include "esp_netif.h"

// #include "lwip/err.h"
// #include "lwip/sockets.h"
// #include "lwip/sys.h"
// #include "lwip/netdb.h"
// #include "lwip/dns.h"
// #include "esp_tls.h"
#include "esp_crt_bundle.h"


// #include "esp_crt_bundle.h"
// #include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "SPIFFS.h"

#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
extern const uint8_t x509_crt_imported_bundle_bin_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_imported_bundle_bin_end[]   asm("_binary_x509_crt_bundle_end");
// #else
// // extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_bin_start");
// // extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
// // extern const uint8_t rootca_crt_bundle_pem_start[] asm("_binary_ca_cert_pem_start");
// // extern const uint8_t rootca_crt_bundle_start[] asm("_binary_ca_cert_pem_start");
// extern const uint8_t rootca_crt_bundle_start[] asm("_binary_ca_certs_bin_start");
// extern const uint8_t rootca_crt_bundle_end[] asm("_binary_ca_certs_bin_end");
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE

namespace ota_update {

    const char * const TAG = "ota_update";

#ifdef SERVER_OTA_UPDATE_URL
    // const char* getRootCACertificate() {        
    //     // static std::string rootCACertificate{""};

    //     // if (rootCACertificate.length() == 0) {
    //     //     File file = SPIFFS.open(CA_CERT_PEM_FILE, "r");

    //     //     if (!file) {
    //     //         CoopLogger::loge(TAG, "Failed to open CA certificate for reading");
    //     //     } else {
    //     //         file.readBytes(rootCACertificate, file.size());
    //     //     }

    //     //     file.close();
    //     // }
        
    //     // return rootCACertificate.c_str();
    //     return (char *)rootca_crt_bundle_start;
    // }

    esp_err_t updateFromUrl(const char* url) {     
// #ifndef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE             
        // esp_crt_bundle_set(rootca_crt_bundle_start, rootca_crt_bundle_end - rootca_crt_bundle_start);    
// #endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE   
        esp_http_client_config_t config = {
            .url = url,
            // .cert_pem = (char *)rootca_crt_bundle_pem_start //getRootCACertificate(),
            // .transport_type = HTTP_TRANSPORT_OVER_SSL,
            .crt_bundle_attach = esp_crt_bundle_attach 
            // .crt_bundle_attach = arduino_esp_crt_bundle_attach
        };
        CoopLogger::logi(TAG, "Starting OTA firmware update from URL: %s", url);
        return esp_https_ota(&config);
#else
        return ESP_FAIL;
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
    }


    void checkUrlForUpdate() {
        // mbedtls_ssl_config conf;
        // mbedtls_ssl_config_init(&conf);
        // esp_crt_bundle_attach(&conf);
        

        WiFiClientSecure client;
        // class WiFiClientSecureRootBundle : public WiFiClientSecure {
        //     public:
        //         WiFiClientSecureRootBundle() : WiFiClientSecure() { _use_ca_bundle = true; };
        // } client;
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE           
        client.setCACertBundle(x509_crt_imported_bundle_bin_start);
// #else
//         client.setCACertBundle(rootca_crt_bundle_start);
#endif // CONFIG_MBEDTLS_CERTIFICATE_BUNDLE        
        // client.setCACert((char*)rootca_crt_bundle_pem_start);
        HTTPClient http;
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        CoopLogger::logd(TAG, "SERVER_OTA_UPDATE_URL: %s", SERVER_OTA_UPDATE_URL);
        http.begin(client, SERVER_OTA_UPDATE_URL);
        int errorCode = http.GET();
        if (errorCode != HTTP_CODE_OK) {
            CoopLogger::loge(TAG, "HTTP GET failed, error: %s (%d)", http.errorToString(errorCode).c_str(), errorCode);
            return;
        }
        String jsonFile = http.getString();
        CoopLogger::logd(TAG, "jsonFile: %s", jsonFile.c_str());
        http.end();

        // parson json file
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, jsonFile);
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
                std::this_thread::sleep_for(std::chrono::seconds(5));
                esp_restart();
            } else {
                CoopLogger::loge(TAG, "Firmware upgrade failed, error: %s", esp_err_to_name(ret));
        }
        } else {
            CoopLogger::logd(TAG, "Current version: '%s' is not older than server version: '%s', skipping", VERSION_BUILD, version);
            return;
        }
        
    }  

    void serverOtaHandle(void * pvParameters) {
        while(true) {
            while (WiFi.status() != WL_CONNECTED) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
            }
            checkUrlForUpdate();
            std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_OTA_CHECK_INTERVAL_SECS * 1000));
        }
    }
#endif // SERVER_OTA_UPDATE_URL  

    void devOtaHandle() {
        
        while (WiFi.status() != WL_CONNECTED) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        // set up direct OTA
        ArduinoOTA.setHostname(HOSTNAME);
        ArduinoOTA.setPassword(DEV_OTA_UPDATE_PASSWORD);
        ArduinoOTA
            .onStart([]() {
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";
                
                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                CoopLogger::logi(TAG, "Start updating %s", type);
                })
            .onEnd([]() {
                CoopLogger::logi(TAG, "\nEnd");
                })
            .onProgress([](unsigned int progress, unsigned int total) {
                CoopLogger::getDefaultPrintStream()->printf("Progress: %u%%\r", (progress / (total / 100)));
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
        while(true) {
            ArduinoOTA.handle();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    void init() {
        std::thread devOta(devOtaHandle);
        devOta.detach();

#ifdef SERVER_OTA_UPDATE_URL
        // std::thread serverOta(serverOtaHandle);
        // serverOta.detach();
        xTaskCreate(serverOtaHandle,          /* Task function. */
                "serverOtaHandle",        /* String with name of task. */
                10000,            /* Stack size in bytes. */
                NULL,             /* Parameter passed as input of the task */
                1,                /* Priority of the task. */
                NULL);
#endif // SERVER_OTA_UPDATE_URL                
    }
} // namespace ota_update


#endif // OTA_UPDATE_CPP_