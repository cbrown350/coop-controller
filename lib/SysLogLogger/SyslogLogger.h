#ifndef SYSLOG_LOGGER_H_
#define SYSLOG_LOGGER_H_

#include <WiFiUdp.h>
#include "settings.h"

#include <Logger.h>

#include <Syslog.h>

class SyslogLogger : public Logger<> {
    public:
        ~SyslogLogger() override = default;
        explicit SyslogLogger(const char* server = SYSLOG_SERVER, uint16_t port = SYSLOG_PORT, uint8_t protocol = SYSLOG_PROTO_BSD)
#if defined(SYSLOG_SERVER) && defined(ENABLE_LOGGING)       
                : Logger(), 
                    udpClient(),
                    syslog(udpClient, protocol) {
            // TODO: use HasData to allow dynamic setting of server/port
            syslog.server(server, port);
            syslog.deviceHostname(HOSTNAME);
//            syslog.appName(PRODUCT_NAME);
            syslog.defaultPriority(LOG_KERN);
            Logger::logi("syslog", "Sending SysLogs to %s:%d", SYSLOG_SERVER, SYSLOG_PORT);
        };

    private:
        WiFiUDP udpClient;
        mutable Syslog syslog;

    // Logger offers:
    //   LOG_EMERG 0 - system is unusable */
    //   LOG_ALERT 1 - action must be taken immediately */
    //   LOG_CRIT  2 - critical conditions */
    //   *LOG_ERR   3 - error conditions */
    //   *LOG_WARNING 4 - warning conditions */
    //   LOG_NOTICE  5 - normal but significant condition */
    //   *LOG_INFO  6 - informational */
    //   *LOG_DEBUG 7 - debug-level messages */
    // only * implemented

        static bool isConnected() {
            if ((WiFiGenericClass::getMode() == WIFI_STA || WiFiGenericClass::getMode() == WIFI_AP_STA)
                    && WiFiSTAClass::status() == WL_CONNECTED)
                    return true;
            // TODO: store up to so many messages and relay them when connected?
            getDefaultPrintStream()->println("[syslog] Syslog not connected");
            return false;
        }
        
        void v_logv(const char *msg) const override {
            if(isConnected())
                v_logd(msg); // no Syslog logger for verbose, using debug
        };

        void v_logd(const char *msg) const override {
            if(isConnected())
                syslog.log(LOG_DEBUG,  msg);
        };


        void v_logi(const char *msg) const override {
            if(isConnected())
                syslog.log(LOG_INFO,  msg);
        };

        void v_logw(const char *msg) const override {
            if(isConnected())
                syslog.log(LOG_WARNING,  msg);
        };


        void v_loge(const char *msg) const override {
            if(isConnected())
                syslog.log(LOG_ERR,  msg);
        };
#else
                : Logger() {};       
#endif // SYSLOG_SERVER && ENABLE_LOGGING    
};


#endif // SYSLOG_LOGGER_H_