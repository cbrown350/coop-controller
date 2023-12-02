#ifndef SYSLOG_COOPLOGGER_H_
#define SYSLOG_COOPLOGGER_H_

#include "coop_settings.h"
#include "CoopLogger.h"
#include <HardwareSerial.h>
#include <Syslog.h>
#include <WiFiUdp.h>
#include <memory>


class SyslogCoopLogger : public CoopLogger { 
    public:
        ~SyslogCoopLogger() override = default;
        explicit SyslogCoopLogger() 
#if defined(SYSLOG_SERVER) && defined(ENABLE_LOGGING)       
                : CoopLogger(), 
                    udpClient(),
                    syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, HOSTNAME, PRODUCT_NAME, LOG_KERN) {
            syslog.log(LOG_INFO,  "Using SyslogCoopLogger");
        };

    private:
        WiFiUDP udpClient;
        Syslog syslog;

        // Syslog offers:
        //   LOG_EMERG 0 /* system is unusable */
        //   LOG_ALERT 1 /* action must be taken immediately */
        //   LOG_CRIT  2 /* critical conditions */
        //   *LOG_ERR   3 /* error conditions */
        //   *LOG_WARNING 4 /* warning conditions */
        //   LOG_NOTICE  5 /* normal but significant condition */
        //   *LOG_INFO  6 /* informational */
        //   *LOG_DEBUG 7 /* debug-level messages */
        // only * implemented

        bool isConnected() {
            return (WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA) 
                    && WiFi.status() == WL_CONNECTED;
        }
        
        void v_logv(const char *msg) override {
            if(isConnected())
                v_logd(msg); // no Syslog logger for verbose, using debug
        };

        void v_logd(const char *msg) override {
            if(isConnected())
                syslog.log(LOG_DEBUG,  msg);
        };


        void v_logi(const char *msg) override {
            if(isConnected())
                syslog.log(LOG_INFO,  msg);
        };

        void v_logw(const char *msg) override {
            if(isConnected())
                syslog.log(LOG_WARNING,  msg);
        };


        void v_loge(const char *msg) override {
            if(isConnected())
                syslog.log(LOG_ERR,  msg);
        };
#else
                : CoopLogger() {};       
#endif // SYSLOG_SERVER && ENABLE_LOGGING    
};


#endif // SYSLOG_COOPLOGGER_H_