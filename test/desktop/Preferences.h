#ifndef _PREFERENCES_H
#define _PREFERENCES_H

#include <string>
#include <map>
#include <cstring>

#include <Logger.h>

/*
 * This is a fake for the ESP32 Preferences library
 */
class Preferences {
    public:
        inline static const char *TAG = "Preferences";

        Preferences() = default;
        ~Preferences() = default;

        bool begin(const char *namespaceStr, const bool readOnly = false) {
            _namespaceStr = namespaceStr;
            _readOnly = readOnly;
            opened = true;
            return true;
        }

        bool clear() {
            if(!opened) {
                Logger<>::loge(TAG, "Preferences not opened");
                return false;
            }
            if(_readOnly) {
                Logger<>::loge(TAG, "Preferences is read-only");
                return false;
            }
            prefs.erase(_namespaceStr);
            return opened;
        }

        void end() {
            opened = false;
        }

        unsigned putString(const char *key, const char *value) {
            if(!opened) {
                Logger<>::loge(TAG, "Preferences not opened");
                return 0;
            }
            if(_readOnly) {
                Logger<>::loge(TAG, "Preferences is read-only");
                return 0;
            }
            prefs[_namespaceStr][key] = value;
            return strlen(value);
        }

        std::string getString(const char *key, const std::string &defaultRet = "") {
            return !isKey(key) ? defaultRet : prefs[_namespaceStr].at(key);
        }

        bool isKey(const char *key) {
            if(!opened) {
                Logger<>::loge(TAG, "Preferences not opened");
                return false;
            }
            return prefs[_namespaceStr].find(key) != prefs[_namespaceStr].end();
        }

        bool remove(const char * key) {
            if(!opened) {
                Logger<>::loge(TAG, "Preferences not opened");
                return false;
            }
            if(_readOnly) {
                Logger<>::loge(TAG, "Preferences is read-only");
                return false;
            }
            prefs[_namespaceStr].erase(key);
            return true;
        }

private:
        inline static std::map<std::string, std::map<std::string, std::string>> prefs;

        std::string _namespaceStr;
        bool _readOnly = false;
        bool opened = false;
};

#endif