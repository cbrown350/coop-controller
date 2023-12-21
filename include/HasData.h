#ifndef HAS_DATA_H_
#define HAS_DATA_H_

#include <mutex>
#include <map>
#include <string>
#include <Preferences.h>
#include "CoopLogger.h"

template<typename K = std::string, typename D = std::string, typename T = std::map<K, D>>
class HasData {
    public:
        static constexpr const char * TAG{"hsdta"};

        virtual ~HasData() = default;

        [[nodiscard]] virtual const std::string& getInstanceID() = 0;

        [[nodiscard]] virtual const std::vector<std::string> &getNVSIDs() const { static const std::vector<std::string> empty = {}; return empty; }
        [[nodiscard]] virtual const char *getNVSNS() const { return "NONE"; }

        virtual bool loadNVSData() {
            CoopLogger::logv(TAG, "[loadNVSData] for %s", getNVSNS());

            if(getNVSIDs().empty()) {
                CoopLogger::logw(TAG, "No NVS IDs defined for %s", getNVSNS());
                return false;
            }

            if(!nvs.begin(getNVSNS(), true)) {
                if(!nvs.begin(getNVSNS(), false)) {
                    CoopLogger::loge(TAG, "Failed to open NVS (rw) for init namespace %s", getNVSNS());
                    return false;
                }
                nvs.end();
                if(!nvs.begin(getNVSNS(), true)) {
                    CoopLogger::loge(TAG, "Failed to open NVS (ro) after successful init namespace %s", getNVSNS());
                    return false;
                }
            }

            std::scoped_lock l{_dataMutex};
            for(auto &key : getNVSIDs()) {
                if(nvs.isKey(key.c_str())) {
                    _data[key] = nvs.getString(key.c_str()).c_str();
                    CoopLogger::logv(TAG, "Loaded %s = %s", key.c_str(), _data[key].c_str());
                }
            }

            nvs.end();
            return true;
        }

        virtual bool saveNVSData() {
            CoopLogger::logv(TAG, "[saveNVSData] for %s", getNVSNS());

            if(getNVSIDs().empty()) {
                CoopLogger::logw(TAG, "No NVS IDs defined for %s", getNVSNS());
                return false;
            }

            if(!nvs.begin(getNVSNS(), false)) {
                CoopLogger::loge(TAG, "Failed to open (rw) NVS namespace %s", getNVSNS());
                return false;
            }

            std::scoped_lock l{_dataMutex};
            for(auto &key : getNVSIDs()) {
                if(_data.find(key) != _data.end()) {
                    CoopLogger::logv(TAG, "Saving %s = %s", key.c_str(), _data[key].c_str());
                    nvs.putString(key.c_str(), _data[key].c_str());
                }
            }

            nvs.end();
            return true;
        }

        virtual bool deleteNVSData() {
            CoopLogger::logv(TAG, "[deleteNVSData] for %s", getNVSNS());

            if(!nvs.begin(getNVSNS(), false)) {
                CoopLogger::loge(TAG, "Failed to open (rw) NVS namespace %s", getNVSNS());
                return false;
            }

            if(!nvs.clear()) {
                CoopLogger::loge(TAG, "Failed to clear NVS namespace %s", getNVSNS());
                nvs.end();
                return false;
            }

            nvs.end();

            return true;
        }

        std::scoped_lock<std::mutex> getDataLock() const {
            return std::scoped_lock{_dataMutex};
        }

        // getters
        virtual T getData() {
            std::scoped_lock l{_dataMutex};
            return _data;
        }

        virtual T& getDataRef() { return _data; }

        virtual D get(const K &key) {
            std::scoped_lock l{_dataMutex};
            if(_data.find(key) == _data.end()) {
                static const D empty{};
                return empty;
            }
            return _data.at(key);
        }
//        virtual bool getBool(const K &key) const {
//            return _data.find(key) != _data.end() &&
//                    (_data.at(key) == "true" || _data.at(key) == "True");
//        }
//        virtual int getInt(const K &key) const {
//            if(_data.find(key) == _data.end())
//                return 0;
//            return std::stoi(_data.at(key));
//        }
//        virtual float getFloat(const K &key) const {
//            if(_data.find(key) == _data.end())
//                return 0;
//            return std::stof(_data.at(key));
//        }
//        virtual double getDouble(const K &key) const {
//            if(_data.find(key) == _data.end())
//                return 0;
//            return std::stod(_data.at(key));
//        }

        // setters
        virtual void setData(const T &data) {
            std::scoped_lock l{_dataMutex};
            for(auto [key, value] : data) {
                _data[key] = value;
            }
        }

//        template<typename X>
//        void set(const K &key, const X value) {
//            std::scoped_lock l{_dataMutex};
//            _data[key] = std::to_string(value);
//        }
        virtual void set(const K &key, const std::string &value) {
            std::scoped_lock l{_dataMutex};
            _data[key] = value;
        }
//        virtual void set(const K &key, const char *value) {
//            std::scoped_lock l{_dataMutex};
//            _data[key] = std::string(value);
//        }
//        virtual void set(const K &key, const bool value) {
//            std::scoped_lock l{_dataMutex};
//            _data[key] = value ? "true" : "false";
//        }
    protected:
        mutable std::mutex _dataMutex;
        T _data{};
        Preferences nvs{};
};


#endif // HAS_DATA_H_