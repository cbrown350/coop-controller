#ifndef SIMPLELOGGER_H
#define SIMPLELOGGER_H

#include <cstdio>
#include <HardwareSerial.h>
#include <Print.h>
#include "settings.h"
#include <Logger.h>

#ifdef LOG_LEVEL
#define SIMPLE_PRINTSTREAM_DEFAULT_LEVEL LOG_LEVEL
#else
#define SIMPLE_PRINTSTREAM_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

#define DEFAULT_PRINTSTREAM_TAG "deflt"

class SimpleLogger : public Logger<> {
public:
    ~SimpleLogger() override = default;
    explicit SimpleLogger(const bool showTime = true, HardwareSerial &printStream=Serial, const unsigned defaultPrintStreamLevel=SIMPLE_PRINTSTREAM_DEFAULT_LEVEL)
#if defined(ENABLE_LOGGING)
            : Logger(), showTime(showTime), printStream(&printStream), loggerPrinter(this) {
        _defaultPrintStreamLevel = defaultPrintStreamLevel;
        printStream.begin(SERIAL_BAUD_RATE);
        setDefaultPrintStream(&loggerPrinter);
        printLog(DEFAULT_PRINTSTREAM_TAG, _defaultPrintStreamLevel, "[" DEFAULT_PRINTSTREAM_TAG "] Using SimpleLogger");
    };

private:
    class SimpleLoggerPrinter : public Print {
    public:
        explicit SimpleLoggerPrinter(SimpleLogger *simpleLogger) : Print(), simpleLogger(simpleLogger) { };
        ~SimpleLoggerPrinter() override = default;

        size_t write(const uint8_t c) override {
            if(_defaultPrintStreamLevel == LOG_LEVEL_NONE)
                return 0;
            unsigned long now = millis();
            if(c != '\n' && c != '\r') // remove newline since it will be added when sent to logger
                _buffer[bufferIdx++] = c;
            if (bufferIdx >= BUFFER_SIZE-1 || c == '\n' || c == '\r' || c == '\0' || (!flushed && now - last_write_time > BUFFER_FLUSH_MS)) {
                _buffer[bufferIdx] = '\0';
                simpleLogger->printLog(DEFAULT_PRINTSTREAM_TAG, _defaultPrintStreamLevel,
                                       reinterpret_cast<const char *>(_buffer));
                bufferIdx = 0;
                flushed = true;
            } else {
                flushed = false;
            }
            last_write_time = now;
            return 1;
        };

        size_t write(const uint8_t *buffer, const size_t size) override {
            if(_defaultPrintStreamLevel == LOG_LEVEL_NONE)
                return 0;
            for(unsigned i = 0; i < size; i++)
                write(buffer[i]);
            return size;
        };

        int availableForWrite() override { return 1; };

    private:
        SimpleLogger *simpleLogger;
        constexpr static int BUFFER_FLUSH_MS = 5;
        constexpr static int BUFFER_SIZE = 150;
        unsigned char _buffer[BUFFER_SIZE]{0};
        unsigned bufferIdx = 0;
        unsigned long last_write_time = 0;
        bool flushed = false;
    };

    bool showTime = true;
    HardwareSerial *printStream;
    inline static unsigned _defaultPrintStreamLevel = LOG_LEVEL_INFO;
    SimpleLoggerPrinter loggerPrinter{this};

    void printLog(const char * tag, const unsigned level, const char * msg) {
        // create new char array with tag in brackets and msg
        char msg_with_tag[LOG_TAG_MAX_LEN + 3 + strlen(msg) + 1]{};
        sprintf((char*)msg_with_tag, "[%s] %s", tag, msg);
        switch(level) { // can't call static logx methods since they lock the mutex
            case LOG_LEVEL_NONE:
                return;
            case LOG_LEVEL_ERROR:
                v_loge(msg_with_tag);
                break;
            case LOG_LEVEL_WARN:
                v_logw(msg_with_tag);
                break;
            case LOG_LEVEL_INFO:
                v_logi(msg_with_tag);
                break;
            case LOG_LEVEL_DEBUG:
                v_logd(msg_with_tag);
                break;
            case LOG_LEVEL_VERBOSE:
                v_logv(msg_with_tag);
                break;
            default:
                v_logi(msg_with_tag);
                break;
        }
    }

    inline static char time_str[16];
    static const char *getTime() {
        unsigned long ms = millis();//(xthal_get_ccount() / 240) * 1000;
        // sprintf(time_str, "%6lu", ms); // up to 12 chars
        // days:hours:minutes:seconds.milliseconds // 16 chars
        sprintf(time_str, "%02lu:%02lu:%02lu:%02lu.%03lu", ms / 86400000, (ms / 3600000) % 24, (ms / 60000) % 60, (ms / 1000) % 60, ms % 1000);
        return time_str;
    }

    void v_logv(const char *msg) const override {
        if(showTime)
            printStream->printf("%s [VERB] %s\n", getTime(), msg);
        else
            printStream->printf("[VERB] %s\n", msg);
    };

    void v_logd(const char *msg) const override {
        if(showTime)
            printStream->printf("%s [DEBUG] %s\n", getTime(), msg);
        else
            printStream->printf("[DEBUG] %s\n", msg);
    };


    void v_logi(const char *msg) const override {
        if(showTime)
            printStream->printf("%s [INFO] %s\n", getTime(), msg);
        else
            printStream->printf("[INFO] %s\n", msg);
    };

    void v_logw(const char *msg) const override {
        if(showTime)
            printStream->printf("%s [WARN] %s\n", getTime(), msg);
        else
            printStream->printf("[WARN] %s\n", msg);
    };


    void v_loge(const char *msg) const override {
        if(showTime)
            printStream->printf("%s [ERROR] %s\n", getTime(), msg);
        else
            printStream->printf("[ERROR] %s\n", msg);
    };
#else
    : Logger() { void showTime; void printStream; void defaultPrintStreamLevel; };
#endif // ENABLE_LOGGING
};


#endif //SIMPLELOGGER_H
