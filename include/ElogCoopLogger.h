#ifndef ELOG_COOPLOGGER_H_
#define ELOG_COOPLOGGER_H_

#include "coop_settings.h"
#include "CoopLogger.h"
#include <HardwareSerial.h>
#include <Elog.h>
#include <memory>

class ElogCoopLogger : public CoopLogger { 
    public:
        explicit ElogCoopLogger(HardwareSerial &printStream=Serial) 
                : CoopLogger(), printStream(printStream), elog(), coopLoggerPrinter(elog) {
#ifdef ENABLE_LOGGING    
            printStream.begin(SERIAL_BAUD_RATE);
            elog.addSerialLogging(printStream, "CoLo", DEBUG);
            setDefaultPrintStream(&coopLoggerPrinter);
            elog.log(INFO, "Using Elog_CoopLogger");
#endif
        };
        ~ElogCoopLogger() override = default;

    private:
        class ElogCoopLoggerPrinter : public Print {
            public:
                ElogCoopLoggerPrinter(Elog &elog) : Print(), elog(elog) { };
                ~ElogCoopLoggerPrinter() override = default;
                size_t write(uint8_t c) override {
                    long currtime = millis();
                    if (bufferIdx >= BUFFER_SIZE || c == '\n' || c == '\r' || currtime - last_flush_time > 1000) {
                        elog.log(INFO, "%s", buffer); 
                        bufferIdx = 0;
                        last_flush_time = currtime;
                    } else {
                        buffer[bufferIdx++] = c;
                    }
                    return 1;   
                };
                
                size_t write(const uint8_t *buffer, size_t size) override {
                    elog.log(INFO, "%s", buffer); 
                    return size;
                };      

                int availableForWrite() override { return 1; };

            private:
                static const int BUFFER_SIZE = 150;
                Elog &elog;          
                char buffer[BUFFER_SIZE];
                int bufferIdx = 0;
                long last_flush_time = 0;
        };

        HardwareSerial &printStream;
        Elog elog;
        ElogCoopLoggerPrinter coopLoggerPrinter;

        // Elog offers:
        //  EMERGENCY 0
        //  ALERT
        //  CRITICAL
        //  *ERROR
        //  *WARNING
        //  NOTICE
        //  *INFO
        //  *DEBUG 7
        // only * implemented
        
        void v_logv(const char *msg) override {
            v_logd(msg); // no Elog logger for verbose, using debug
        };

        void v_logd(const char *msg) override {
            elog.log(DEBUG, msg);
        };


        void v_logi(const char *msg) override {
            elog.log(INFO, msg);
        };

        void v_logw(const char *msg) override {
            elog.log(WARNING, msg);
        };


        void v_loge(const char *msg) override {
            elog.log(ERROR, msg);
        };
};


#endif // ELOG_COOPLOGGER_H_