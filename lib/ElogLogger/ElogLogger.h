#ifndef ELOG_LOGGER_H_
#define ELOG_LOGGER_H_

#include <HardwareSerial.h>
#include <Print.h>

#include <Logger.h>

#include <Elog.h>

#include <memory>

#ifdef LOG_LEVEL
#define ELOG_PRINTSTREAM_DEFAULT_LEVEL LOG_LEVEL
#else
#define ELOG_PRINTSTREAM_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

#define DEFAULT_PRINTSTREAM_TAG "deflt"

class ElogLogger : public Logger<> {
    public:
        ~ElogLogger() override = default;
        explicit ElogLogger(HardwareSerial &printStream=Serial, const unsigned defaultPrintStreamLevel=ELOG_PRINTSTREAM_DEFAULT_LEVEL)
#ifdef ENABLE_LOGGING    
                : Logger(), elog(), loggerPrinter(elog) {
            setDefaultPrintStreamLevel(defaultPrintStreamLevel);
            printStream.begin(SERIAL_BAUD_RATE);
            elog.addSerialLogging(printStream, "", DEBUG); // already filtering out log levels elsewhere, use highest here
            setDefaultPrintStream(&loggerPrinter);
            elog.log(_defaultPrintStreamLevel, "[" DEFAULT_PRINTSTREAM_TAG "] Using ElogLogger");
        };

        static void setDefaultPrintStreamLevel(const unsigned level) {
            _defaultPrintStreamLevel = translateLogLevelToElog(level);
        };

    private:
//          ELog Levels:
//            EMERGENCY = 0,
//            ALERT = 1,
//            CRITICAL = 2,
//            ERROR = 3,
//            WARNING = 4,
//            NOTICE = 5,
//            INFO = 6,
//            DEBUG = 7,
        static Loglevel translateLogLevelToElog(const unsigned level) {
            switch(level) {
                case LOG_LEVEL_NONE:
                    return EMERGENCY;
                case LOG_LEVEL_ERROR:
                    return ERROR;
                case LOG_LEVEL_WARN:
                    return WARNING;
                case LOG_LEVEL_INFO:
                    return INFO;
                case LOG_LEVEL_DEBUG:
                case LOG_LEVEL_VERBOSE:
                    return DEBUG;
                default:
                    return INFO;
            }
        }

        class ElogLoggerPrinter : public Print {
            public:
                explicit ElogLoggerPrinter(Elog &elog) : Print(), elog(elog) { };
                ~ElogLoggerPrinter() override = default;

                size_t write(uint8_t c) override {
                    if(_defaultPrintStreamLevel == EMERGENCY) // equivalent of NONE
                        return 0;
                    unsigned long now = millis();
                    if(c != '\n' && c != '\r') // remove newline since it will be added when sent to logger
                        _buffer[bufferIdx++] = c;
                    if (bufferIdx >= BUFFER_SIZE-1 || c == '\n' || c == '\r' || (!flushed && now - last_write_time > BUFFER_FLUSH_MS)) {
                        _buffer[bufferIdx] = '\0';
                        elog.log(_defaultPrintStreamLevel, "[" DEFAULT_PRINTSTREAM_TAG "] %s", _buffer);
                        bufferIdx = 0;
                        flushed = true;
                    } else {
                        flushed = false;
                    }
                    last_write_time = now;
                    return 1;   
                };
                
                size_t write(const uint8_t *buffer, size_t size) override {
                    if(_defaultPrintStreamLevel == EMERGENCY) // equivalent of NONE
                        return 0;
//                    elog.log(_defaultPrintStreamLevel, "[" DEFAULT_PRINTSTREAM_TAG "] %s", buffer);
                    for(unsigned i = 0; i < size; i++)
                        write(buffer[i]);
                    return size;
                };      

                int availableForWrite() override { return 1; };

            private:
                constexpr static int BUFFER_FLUSH_MS = 5;
                constexpr static int BUFFER_SIZE = 150;
                Elog &elog;          
                unsigned char _buffer[BUFFER_SIZE]{0};
                unsigned bufferIdx = 0;
                unsigned long last_write_time = 0;
                bool flushed = false;
        };

        inline static Loglevel _defaultPrintStreamLevel = INFO;
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