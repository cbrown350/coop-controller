//
// Created by cbrow on 2023-12-26.
//

#ifndef CODE_SIMPLELOGGER_H
#define CODE_SIMPLELOGGER_H

#include "settings.h"
#include <Logger.h>

class SimpleLogger : public Logger<> {
public:
    ~SimpleLogger() override = default;
    explicit SimpleLogger(const bool showTime = true)
#if defined(ENABLE_LOGGING)
            : Logger(), showTime(showTime) { Serial.begin(SERIAL_BAUD_RATE); };

private:
    bool showTime = true;

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

    const char *getTime() const {
        static char time[20];
        unsigned long ms = millis();//(xthal_get_ccount() / 240) * 1000;
        sprintf(time, "%lu", ms);
        return time;
    }

    void v_logv(const char *msg) const override {
        if(showTime)
            defaultPrintStream->printf("%s [VERB] %s\n", getTime(), msg);
        else
            defaultPrintStream->printf("[VERB] %s\n", msg);
    };

    void v_logd(const char *msg) const override {
        defaultPrintStream->printf("[DEBUG] %s\n", msg);
    };


    void v_logi(const char *msg) const override {
        defaultPrintStream->printf("[INFO] %s\n", msg);
    };

    void v_logw(const char *msg) const override {
        defaultPrintStream->printf("[WARN] %s\n", msg);
    };


    void v_loge(const char *msg) const override {
        defaultPrintStream->printf("[ERROR] %s\n", msg);
    };
#else
    : Logger() { void showTime; };
#endif // ENABLE_LOGGING
};


#endif //CODE_SIMPLELOGGER_H
