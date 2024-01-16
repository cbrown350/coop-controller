#ifndef CODE_LOGGER_H
#define CODE_LOGGER_H

#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <iomanip>
#include <chrono>

#include <Logger.h>

#if defined(ARDUINO) || defined(ESP32)
#pragma GCC error "This header should not be included in embedded"
#endif

#ifndef LOG_LEVEL_NONE
#define LOG_LEVEL_NONE 0
#endif
#ifndef LOG_LEVEL_ERROR
#define LOG_LEVEL_ERROR 1
#endif
#ifndef LOG_LEVEL_WARN
#define LOG_LEVEL_WARN 2
#endif
#ifndef LOG_LEVEL_INFO
#define LOG_LEVEL_INFO 3
#endif
#ifndef LOG_LEVEL_DEBUG
#define LOG_LEVEL_DEBUG 4
#endif
#ifndef LOG_LEVEL_VERBOSE
#define LOG_LEVEL_VERBOSE 5
#endif

class DesktopLogger : public Logger<>  {
protected:
    static void printPre() {
        std::cout << std::setfill('0') << std::setw(timePadding) << millis();
    }

public:
    inline static unsigned timePadding = 5;
    static unsigned long millis() {
        const auto time = std::chrono::system_clock::now();
        const auto time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
        static const unsigned long startTime = time_ms;
        return time_ms - startTime;
    }

    static void logv(const char *tag, const char *format, ...) {
#if defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_VERBOSE
        printPre();
        std::cout << " [VERB] [" << tag << "] ";
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        std::cout << std::endl;
#endif
    }

    static void logd(const char *tag, const char *format, ...) {
#if defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_DEBUG
        printPre();
        std::cout << " [DEBUG] [" << tag << "] ";
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        std::cout << std::endl;
#endif
    }

    static void logi(const char *tag, const char *format, ...) {
#if defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_INFO
        printPre();
        std::cout << " [INFO] [" << tag << "] ";
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        std::cout << std::endl;
#endif
    }

    static void logw(const char *tag, const char *format, ...) {
#if defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_WARN
        printPre();
        std::cout << " [WARN] [" << tag << "] ";
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        std::cout << std::endl;
#endif
    }

    static void loge(const char *tag, const char *format, ...) {
#if defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_ERROR
        printPre();
        std::cout << " [ERROR] [" << tag << "] ";
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        std::cout << std::endl;
#endif
    }

    void v_logv(const char *msg) const override {
        printPre();
        std::cout << " [VERB] " << msg << std::endl;
    }

    void v_logd(const char *msg) const override {
        printPre();
        std::cout << " [DEBUG] " << msg << std::endl;
    }

    void v_logi(const char *msg) const override {
        printPre();
        std::cout << " [INFO] " << msg << std::endl;
    }

    void v_logw(const char *msg) const override {
        printPre();
        std::cout << " [WARN] " << msg << std::endl;
    }

    void v_loge(const char *msg) const override {
        printPre();
        std::cout << " [ERROR] " << msg << std::endl;
    }
};

#endif //CODE_LOGGER_H
