//
// Created by cbrow on 2023-12-26.
//

#ifndef CODE_TESTLOGGER_H
#define CODE_TESTLOGGER_H

#include "settings.h"

#include <HardwareSerial.h>
#include <Logger.h>

#include <string.h>
#include <vector>
#include <algorithm>

class TestLogger : public Logger<> {
public:
    inline static constexpr const char * TAG{"tlog"};
    inline static constexpr unsigned LOG_ENTRIES_TO_SAVE = 100;

    explicit TestLogger() : Logger() {
        static bool initialized = false;
        Serial.begin(SERIAL_BAUD_RATE);
        if(initialized){
            static const std::string msg{"Already initialized TestLogger!"};
            Serial.printf("%s [ERROR] [%s] %s\n", getTime(), TAG, msg.c_str());
            delay(3000);
            throw std::runtime_error("Error: "+msg);
        }
        initialized = true;
        logs = new LogEntry[LOG_ENTRIES_TO_SAVE];
    };    

    ~TestLogger() override {
        delete logs;
    };

    struct LogEntry {
        unsigned long time;
        char level[10];
        char msg[100];
        [[nodiscard]] bool isEmpty() const { return level[0] == '\0'; }
    };

    void printLog(const unsigned index) const {
        auto log = getLog(index);
        if(log.isEmpty()) 
            return;
        defaultPrintStream->printf("%s [%s] [%s] %s\n", getTime(log.time), TAG, log.level, log.msg);
    }

    void printLogs() const {
        defaultPrintStream->printf("    %s [%s] Printing all test logs (%d entries)\n", getTime(), TAG, logArrayFilled ? LOG_ENTRIES_TO_SAVE : nextLogPtr);
        for (unsigned i = 0; (logArrayFilled && i < LOG_ENTRIES_TO_SAVE) || (!logArrayFilled && i < nextLogPtr); i++) 
            printLog(i);
    }

    // Returns that log entry at index, where 0 is the most recent log entry
    //  must be a lower index than LOG_ENTRIES_TO_SAVE
    [[nodiscard]] LogEntry getLog(const unsigned index) const {
        return getLog(index, true);
    }

    // Returns the last numLogs log entries, starting with the most recent log entry
    //  must be fewer than LOG_ENTRIES_TO_SAVE
    [[nodiscard]] std::vector<LogEntry> getLogs(const unsigned numLogs = LOG_ENTRIES_TO_SAVE-1) const {
        std::vector<LogEntry> logs;
        std::scoped_lock l(logMutex);
        for (unsigned i = 0; i < numLogs && i < LOG_ENTRIES_TO_SAVE; i++) {
            const auto &log = getLog(i, false);
            if(log.isEmpty()) 
                break;
            logs.push_back(log);
        }
        return logs;
    }
    
    [[nodiscard]] const char *getTime(const unsigned long time = 0) const {
        static char _time[20];
        if(time != 0) {
            sprintf(_time, "%lu", time);
            return _time;
        }
        sprintf(_time, "%lu", millis());
        return _time;
    }
    
    [[nodiscard]] bool checkMatchesLogEntryMsg(const unsigned logIndex, const char *expectedMsg, const bool onlyStartsWith = true) const {
        const auto msg = getLog(logIndex).msg;
        return onlyStartsWith ? strncmp(msg, expectedMsg, strlen(expectedMsg))==0 : strcmp(msg, expectedMsg)==0;
    }

    [[nodiscard]] bool checkMatchesLogEntryOrBeforeMsg(const unsigned logIndex, const char *expectedMsg, const bool onlyStartsWith = true) const {
        for(unsigned i = logIndex; (logArrayFilled && i < LOG_ENTRIES_TO_SAVE) || (!logArrayFilled && i < nextLogPtr); i++) 
            if(checkMatchesLogEntryMsg(i, expectedMsg, onlyStartsWith))
                return true;
        return false;
    }   

private:
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

    mutable LogEntry *logs;
    mutable unsigned nextLogPtr = 0;
    mutable std::mutex logMutex;
    mutable bool logArrayFilled = false;
    
    // Returns that log entry at index, where 0 is the most recent log entry and optionally locks the logs data
    //  must be a lower index than LOG_ENTRIES_TO_SAVE
    [[nodiscard]] LogEntry getLog(const unsigned index, bool lockData) const {
        if(index >= LOG_ENTRIES_TO_SAVE || (!logArrayFilled && index >= nextLogPtr)) 
            return {};
        std::unique_lock l(logMutex, std::defer_lock);
        if(lockData) 
            l.lock();
        unsigned calcIndex = (nextLogPtr-1 + LOG_ENTRIES_TO_SAVE - index) % LOG_ENTRIES_TO_SAVE;
        return logs[calcIndex];
    }
    
    void storeLog(const unsigned long time, const char *level, const char *msg) const {
        std::scoped_lock l(logMutex);
        logs[nextLogPtr].time = time;
        strcpy(logs[nextLogPtr].level, level);
        strcpy(logs[nextLogPtr].msg, msg);
        if (++nextLogPtr >= LOG_ENTRIES_TO_SAVE) {
            nextLogPtr = 0;
            logArrayFilled = true;
        }
    }

    void doLog(const char *level, const char *msg) const {
        auto time = millis();
        defaultPrintStream->printf("%s [%s] %s\n", getTime(time), level, msg);
        storeLog(time, level, msg);   
    }

    void v_logv(const char *msg) const override {
        doLog("VERB", msg);
    }

    void v_logd(const char *msg) const override {
        doLog("DEBUG", msg);
    }


    void v_logi(const char *msg) const override {
        doLog("INFO", msg);
    }

    void v_logw(const char *msg) const override {
        doLog("WARN", msg);
    }


    void v_loge(const char *msg) const override {
        doLog("ERROR", msg);
    }
};


#endif //CODE_TESTLOGGER_H
