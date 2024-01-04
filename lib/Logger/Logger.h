#ifndef LOGGER_H_
#define LOGGER_H_

#if __has_include( "settings.h" )
#include "settings.h"
#endif

#if defined(ARDUINO)
#include <HardwareSerial.h>
#include <Print.h>
#endif

#include <vector>
#include <memory>
#include <mutex>


#ifndef LOG_TAG_MAX_LEN
#define LOG_TAG_MAX_LEN 5
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

#if defined(ARDUINO)
template<class T = Print>
#else
template<class T = std::ostream>
#endif
class Logger {
    protected:
        Logger() = default;

#if defined(ARDUINO)
        inline static T *defaultPrintStream = &Serial;
#else
        inline static T *defaultPrintStream;
#endif

        inline static std::mutex logMutex = std::mutex();

    private:
        inline static std::unique_ptr<std::vector<std::shared_ptr<Logger>>> loggers = nullptr;

        virtual void v_logv(const char *msg) const { /* do nothing if not enabled */ };
        virtual void v_logd(const char *msg) const { /* do nothing if not enabled */ };
        virtual void v_logi(const char *msg) const { /* do nothing if not enabled */ };
        virtual void v_logw(const char *msg) const { /* do nothing if not enabled */ };
        virtual void v_loge(const char *msg) const { /* do nothing if not enabled */ };

        static void dummy(...) { }

    public:
        virtual ~Logger() = default;
        Logger(const Logger& obj) = delete; 
            
        static void setDefaultPrintStream(T *_defaultPrintStream) {
            // TODO: implement wrapper class with mutex locking?
            Logger::defaultPrintStream = _defaultPrintStream;
        }
        static T *getDefaultPrintStream() {
            return defaultPrintStream;
        }

        static void addLogger(const std::shared_ptr<Logger> &logger) {
#ifdef ENABLE_LOGGING
            if(loggers == nullptr)
                loggers = std::make_unique<std::vector<std::shared_ptr<Logger>>>();
            loggers->push_back(std::move(logger));
#else
            (void)logger;            
#endif
        }

        template<typename... Args>            
        static void logv(const char* TAG, const char *msg, Args&&... args) {
#if defined(ENABLE_LOGGING) && defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_VERBOSE
            if(loggers == nullptr || loggers->empty())
                return;
            const std::string tag_msg = std::string("[%-*.*s] ") + msg;
            auto size = std::snprintf(nullptr, 0, tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::string parsed_msg(size + 1, '\0');
            std::sprintf(&parsed_msg[0], tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::scoped_lock l(logMutex);
            for(auto const &logger : *loggers)
                logger->v_logv(parsed_msg.c_str());
#else                
            (void)TAG;
            (void)msg;
//            (void)args;
            dummy(std::forward<Args>(args)...);
#endif
        }

        template<typename... Args>            
        static void logd(const char* TAG, const char *msg, Args&&... args) {
#if defined(ENABLE_LOGGING) && defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_DEBUG
            if(loggers == nullptr || loggers->empty())
                return;
            const std::string tag_msg = std::string("[%-*.*s] ") + msg;
            auto size = std::snprintf(nullptr, 0, tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::string parsed_msg(size + 1, '\0');
            std::sprintf(&parsed_msg[0], tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::scoped_lock l(logMutex);
            for(auto const &logger : *loggers)
                logger->v_logd(parsed_msg.c_str());
#else                
            (void)TAG;
            (void)msg;
//            (void)args;
            dummy(std::forward<Args>(args)...);
#endif
        }
    
        template<typename... Args>            
        static void logi(const char *TAG, const char *msg, Args&&... args) {
#if defined(ENABLE_LOGGING) && defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_INFO
            if(loggers == nullptr || loggers->empty())
                return;
            const std::string tag_msg = std::string("[%-*.*s] ") + msg;
            auto size = std::snprintf(nullptr, 0, tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::string parsed_msg(size + 1, '\0');
            std::sprintf(&parsed_msg[0], tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::scoped_lock l(logMutex);
            for(auto const &logger : *loggers)
                logger->v_logi(parsed_msg.c_str());
#else                
            (void)TAG;
            (void)msg;
//            (void)args;
            dummy(std::forward<Args>(args)...);
#endif
        }

        template<typename... Args>            
        static void logw(const char* TAG, const char *msg, Args&&... args) {
#if defined(ENABLE_LOGGING) && defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_WARN
            if(loggers == nullptr || loggers->empty())
                return;
            const std::string tag_msg = std::string("[%-*.*s] ") + msg;
            auto size = std::snprintf(nullptr, 0, tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::string parsed_msg(size + 1, '\0');
            std::sprintf(&parsed_msg[0], tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::scoped_lock l(logMutex);
            for(auto const &logger : *loggers)
                logger->v_logw(parsed_msg.c_str());
#else                
            (void)TAG;
            (void)msg;
//            (void)args;
            dummy(std::forward<Args>(args)...);
#endif
        }

        template<typename... Args>            
        static void loge(const char* TAG, const char *msg, Args&&... args) {
#if defined(ENABLE_LOGGING) && defined(LOG_LEVEL) && LOG_LEVEL >= LOG_LEVEL_ERROR
            if(loggers == nullptr || loggers->empty())
                return;
            const std::string tag_msg = std::string("[%-*.*s] ") + msg;
            auto size = std::snprintf(nullptr, 0, tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::string parsed_msg(size + 1, '\0');
            std::sprintf(&parsed_msg[0], tag_msg.c_str(), LOG_TAG_MAX_LEN, LOG_TAG_MAX_LEN, TAG, std::forward<Args>(args)...);
            std::scoped_lock l(logMutex);
            for(auto const &logger : *loggers)
                logger->v_loge(parsed_msg.c_str());
#else                
            (void)TAG;
            (void)msg;
//            (void)args;
            dummy(std::forward<Args>(args)...);
#endif
        }       
};

#endif // LOGGER_H_