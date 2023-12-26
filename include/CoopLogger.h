#ifndef COOPLOGGER_H_
#define COOPLOGGER_H_

#include <HardwareSerial.h>
#include <Print.h>
#include <vector>
#include <memory>
#include <mutex>
#include "coop_settings.h"


class CoopLogger {
    protected:
        CoopLogger() = default;
        inline static Print *defaultPrintStream = &Serial;
        inline static std::mutex logMutex = std::mutex();

    private:
        inline static std::unique_ptr<std::vector<std::unique_ptr<CoopLogger>>> loggers = nullptr;

        virtual void v_logv(const char *msg) { /* do nothing if not enabled */ };
        virtual void v_logd(const char *msg) { /* do nothing if not enabled */ };
        virtual void v_logi(const char *msg) { /* do nothing if not enabled */ }; 
        virtual void v_logw(const char *msg) { /* do nothing if not enabled */ };
        virtual void v_loge(const char *msg) { /* do nothing if not enabled */ };

        static void dummy(...) {};

    public:
        virtual ~CoopLogger() = default;
        CoopLogger(const CoopLogger& obj) = delete; 
            
        static void setDefaultPrintStream(Print *_defaultPrintStream) {
            // TODO: implement wrapper class with mutex locking?
            CoopLogger::defaultPrintStream = _defaultPrintStream;
        }
        static Print *getDefaultPrintStream() {
            return defaultPrintStream;
        }

        static void addLogger(std::unique_ptr<CoopLogger> logger) {
#ifdef ENABLE_LOGGING
            if(loggers == nullptr)
                loggers = std::make_unique<std::vector<std::unique_ptr<CoopLogger>>>();
            loggers->push_back(std::move(logger));
#else
            (void)logger;            
#endif
        }

        template<typename... Args>            
        static void logv(const char* TAG, const char *msg, Args&&... args) {
#if defined(ENABLE_LOGGING) && defined(COOP_LOG_LEVEL) && (COOP_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_VERBOSE)
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
#if defined(ENABLE_LOGGING) && defined(COOP_LOG_LEVEL) && COOP_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
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
#if defined(ENABLE_LOGGING) && defined(COOP_LOG_LEVEL) && COOP_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
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
#if defined(ENABLE_LOGGING) && defined(COOP_LOG_LEVEL) && COOP_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN
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
#if defined(ENABLE_LOGGING) && defined(COOP_LOG_LEVEL) && COOP_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
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

#endif // COOPLOGGER_H_