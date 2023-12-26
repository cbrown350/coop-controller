#ifndef HAS_DATA_H_
#define HAS_DATA_H_

#include <mutex>
#include <map>
#include <string>
#include <utility>
#include <Preferences.h>
#include "CoopLogger.h"

/*
 * HasData is a base class for classes that need to store data in NVS and/or provide generic get/set methods for data
 *  withing a subclass or class using an instance, anonymous or not. It handles loading/saving data from/to
 *  non-volatile storage (NVS) and provides methods to get/set data.
 *
 *  Most methods already provid most of the needed implementation, but can be overridden to provide custom
 *  functionality if desired. But pure virtual methods getWithOptLock(), setWithOptLockAndUpdate() and getKeys()
 *  must be implemented in order to provide access to the actual values and determine whether and when the object
 *  updates and whether there is a data mutex lock using provided member variable _dataMutex. This lock and an NVS
 *  mutex lock are used to ensure data is not corrupted when accessed from multiple threads during load/save.
 *
 *  Method updateObj() may be overridden if the using object needs to be updated, ex.: update time from NTP server,
 *  server settings, etc. If overriding updateObj() and the data needs saving to NVS, you need to call super method
 *  HasData::updateObj() from the overridden method or call the NVS save methods directly.  Methods
 *  getReadOnlyKeys() and getNvsKeys() can be overridden to provide read-only keys and keys used to load/save
 *  data from/to NVS, respectively. Method getNvsNamespace() by default uses the instanceID string, which must be
 *  shorter than 15 characters if used as NVS and will be truncated if not. Constructors must set the instanceID,
 *  which may be used for the NVS namespace and to differentiate instances.
 */


class HasData {
    public:
        static constexpr const char * TAG{"hsdta"};

        // instanceID must be shorter than 15 characters if used as NVS namespace
        const std::string instanceID;
        inline static constexpr const char * const EMPTY_VALUE{"-0-"};
        virtual ~HasData() = default;


        [[nodiscard]] const std::string& getInstanceID() const { return instanceID; }

        // load data from the NVS partition in namespace provided from getNvsNamespace() into keys provided from getNvsKeys()
        [[nodiscard]] virtual bool loadNvsData();

        // save data to the NVS partition in namespace provided from getNvsNamespace() into keys provided from getNvsKeys()
        [[nodiscard]] virtual bool saveNvsData(const std::vector<std::string> &keys, bool _dataAlreadyLocked) const;

        [[nodiscard]] virtual bool deleteNvsData() const ;

        [[nodiscard]] virtual std::map<std::string, std::string> getData() const;
        [[nodiscard]] virtual std::string get(const std::string &key) const { return getWithOptLock(key, false); }

        [[nodiscard]] virtual bool setData(const std::map<std::string, std::string> &newData);
        [[nodiscard]] virtual bool set(const std::string &key, const std::string &value) { return setWithOptLockAndUpdate(key, value, true, true); }


        /****** methods that should/must be implemented ******/

        // namespace used to load/save data from/to NVS, must be shorter than 15 characters
        [[nodiscard]] virtual std::string getNvsNamespace() const {
            // truncate instanceID to 15 characters (14 plus null terminator) if too long to use as NVS namespace
            if(instanceID.length() > 14) {
                CoopLogger::logw(TAG, "instanceID %s is longer than 15 characters, truncating", instanceID.c_str());
                return instanceID.substr(0, 14);
            }
            return instanceID;
        }

        // keys used to load/save data from/to NVS
        [[nodiscard]] virtual std::vector<std::string> getNvsKeys() const { return {}; }

        // keys that are read-only and also won't be written to NVS or have affect if set
        [[nodiscard]] virtual std::vector<std::string> getReadOnlyKeys() const { return {}; }

        // keys used to get/set data
        [[nodiscard]] virtual std::vector<std::string> getKeys() const = 0;

    protected:
        static std::mutex nvsDataMutex;
        mutable std::mutex _dataMutex;

        // instanceID must be shorter than 15 characters if used as NVS namespace
        explicit HasData(std::string instanceID) : instanceID(std::move(instanceID)) {}

        /****** methods that should/must be implemented ******/
        [[nodiscard]] virtual std::string getWithOptLock(const std::string &key, bool noLock) const = 0;
        [[nodiscard]] virtual bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, bool noLock, bool doObjUpdate) = 0;

        /* save to NVS and update underlying details of the object (when overridden), ex.: update time from NTP server, server settings, etc.
           _dataMutex should be unlocked when calling this method */
        [[nodiscard]] virtual bool updateObj(const std::vector<std::string> &keys, const bool _dataAlreadyLocked) {
            if(!keys.empty()) {
                return saveNvsData(keys, _dataAlreadyLocked);
            }
            return true;
        }

};


#endif // HAS_DATA_H_