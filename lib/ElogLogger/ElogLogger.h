#ifndef ELOG_LOGGER_H_
#define ELOG_LOGGER_H_

#include <HardwareSerial.h>

#include <Logger.h>

#include <Elog.h>

#include <memory>

class ElogLogger : public Logger<> {
    public:
        ~ElogLogger() override = default;
        explicit ElogLogger(HardwareSerial &printStream=Serial) 
#ifdef ENABLE_LOGGING    
                : Logger(), elog(), loggerPrinter(elog) {
            printStream.begin(SERIAL_BAUD_RATE);
            elog.addSerialLogging(printStream, "CoLo", DEBUG);
            setDefaultPrintStream(&loggerPrinter);
            elog.log(INFO, "Using ElogLogger");
        };

    private:
        class ElogLoggerPrinter : public Print {
            public:
                explicit ElogLoggerPrinter(Elog &elog) : Print(), elog(elog) { };
                ~ElogLoggerPrinter() override = default;

                size_t write(uint8_t c) override {
                    unsigned long currtime = millis();
                    if (bufferIdx >= BUFFER_SIZE || c == '\n' || c == '\r' || currtime - last_flush_time > 1000) {
                        elog.log(INFO, "%s", _buffer);
                        bufferIdx = 0;
                        last_flush_time = currtime;
                    } else {
                        _buffer[bufferIdx++] = c;
                    }
                    return 1;   
                };
                
                size_t write(const uint8_t *buffer, size_t size) override {
                    elog.log(INFO, "%s", buffer); 
                    return size;
                };      

                int availableForWrite() override { return 1; };

            private:
                constexpr static int BUFFER_SIZE = 150;
                Elog &elog;          
                unsigned char _buffer[BUFFER_SIZE]{0};
                unsigned bufferIdx = 0;
                unsigned long last_flush_time = 0;
        };

        mutable Elog elog;
        ElogLoggerPrinter loggerPrinter;

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
        
        void v_logv(const char *msg) const override {
            v_logd(msg); // no Elog logger for verbose, using debug
        };

        void v_logd(const char *msg) const override {
            elog.log(DEBUG, msg);
        };


        void v_logi(const char *msg) const override {
            elog.log(INFO, msg);
        };

        void v_logw(const char *msg) const override {
            elog.log(WARNING, msg);
        };


        void v_loge(const char *msg) const override {
            elog.log(ERROR, msg);
        };
#else   
                : Logger() { };
#endif // ENABLE_LOGGING
};


#endif // ELOG_LOGGER_H_