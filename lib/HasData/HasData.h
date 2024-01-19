#ifndef HAS_DATA_H_
#define HAS_DATA_H_

#include <mutex>
#include <map>
#include <string>
#include <algorithm>
#include <any>
#include <utility>
#include <Logger.h>

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

template<class L = Logger<>>
class HasData {
    public:
        using Logger = L;

        // `instanceID` must be shorter than 15 characters if used as NVS namespace
        const std::string instanceID;
        // Emtpy value ("-0-") to let you know that a value is not set, different from an empty string
        inline static constexpr const char * const EMPTY_VALUE{"-0-"};
        virtual ~HasData() = default;


        [[nodiscard]] const std::string& getInstanceID() const { return instanceID; }

        /* Load data from the NVS partition in namespace provided from `getNvsNamespace()` into keys provided from `getNvsKeys()`.
        *    Keys will only be loaded and overwritten if they already exist in the NVS.  
        *    You may need to manually reinitialize or otherwise setup the object (updateObj()) after this 
        *    since don't want to reload object automatically since may be recursive loop or unnecessarily call `saveNvsData()` */
        [[nodiscard]] virtual bool loadNvsData();

        /* Save data to the NVS partition in namespace provided from `getNvsNamespace()` into keys provided from `getNvsKeys()`.
         *   Empty strings will be saved to NVS, but values that are the same as HasData::EMPTY_VALUE will not be saved. */
        [[nodiscard]] virtual bool saveNvsData(const std::vector<std::string> &keys, bool _dataAlreadyLocked) const;

        [[nodiscard]] virtual bool deleteNvsData() const ;

        [[nodiscard]] virtual std::map<std::string, std::string> getData() const;
        [[nodiscard]] virtual std::string get(const std::string &key) const { return getWithOptLock(key, false); }

        /* Sets the keys/values in newData in based on implemented `setWithOptLockAndUpdate()` and then runs `updateObj()` if changed
         *   which may intern call `saveNvsData()` in `updateObj()` (by default).
         *   Only returns true if the values were changed and the update was successful. */
        [[nodiscard]] virtual bool setData(const std::map<std::string, std::string> &newData);
        /* Sets new value for key based on implemented `setWithOptLockAndUpdate()` and then runs `updateObj()` if changed.
         *   May or may not call `saveNvsData()` in `updateObj()` depending on how it's implemented by the concrete class.
         *   Only returns true if the values were changed and the update was successful. */
        [[nodiscard]] virtual bool set(const std::string &key, const std::string &value) { return setWithOptLockAndUpdate(key, value, false, true); }


        /****** Methods that should/must be implemented ******/

        // Namespace used to load/save data from/to NVS, must be shorter than 15 characters, defaults to `instanceID`
        [[nodiscard]] virtual std::string getNvsNamespace() const {
            // truncate `instanceID` to 15 characters (14 plus null terminator) if too long to use as NVS namespace
            if(instanceID.length() > 14) {
                Logger::logw(getTag(), "instanceID %s is longer than 15 characters, truncating", instanceID.c_str());
                return instanceID.substr(0, 14);
            }
            return instanceID;
        }

        // Keys used to load/save data from/to NVS
        [[nodiscard]] virtual std::vector<std::string> getNvsKeys() const { return {}; }

        // Keys that are read-only and also won't be written to NVS or have affect if set
        [[nodiscard]] virtual std::vector<std::string> getReadOnlyKeys() const { return {"blah"}; }

        // Keys used to get/set data
        [[nodiscard]] virtual std::vector<std::string> getKeys() const = 0;

        [[nodiscard]] virtual const char * getTag() const = 0;


    protected:
        inline static std::mutex nvsDataMutex;
        mutable std::mutex _dataMutex;

        // `instanceID` must be shorter than 15 characters if used as NVS namespace
        explicit HasData(std::string instanceID) : instanceID(std::move(instanceID)) {}

        /****** Methods that should/must be implemented ******/
        [[nodiscard]] virtual std::string getWithOptLock(const std::string &key, bool noLock) const = 0;
        [[nodiscard]] virtual bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, bool noLock, bool doObjUpdate) = 0;

        /* Save to NVS and update underlying details of the object (when overridden), ex.: update time from NTP server, server settings, etc.
             `_dataMutex` may be locked already when calling this method if `_dataAlreadyLocked` is true.
             By default  `saveNvsData()` will be called to store data to NVS unless reimplemented */
        [[nodiscard]] virtual bool updateObj(const std::vector<std::string> &keys, const bool _dataAlreadyLocked) {
            if(!keys.empty()) {
                // check at least one key in keys is in getNvsKeys()
                const auto nvsKeys = getNvsKeys();
                for(const auto &key : keys) {
                    if(std::find(nvsKeys.begin(), nvsKeys.end(), key) != nvsKeys.end())
                        return saveNvsData(keys, _dataAlreadyLocked);
                }
            }
            return true;
        }

        /* Helper method to generically get a string value from a map of key/T value ptr pairs, returns HasData::EMPTY_VALUE if key not found.
         *   `_dataMutex` may be locked already when calling this method if `noLock` is true. */
        template<class T=std::string>
        [[nodiscard]] T getStringDataHelper(const std::map<const std::string, const T&> &keyVarPairs,
                                            const std::string &key, const bool noLock, const T& defaultVal = HasData::EMPTY_VALUE) const {
            if(keyVarPairs.find(key) == keyVarPairs.end()) {
                Logger::logv(getTag(), "Invalid key '%s', not found", key.c_str());
                return defaultVal;
            }

            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
                l.lock();
            return keyVarPairs.at(key);
        }

        class VarHolder {
        public:
            std::any varPtr;
            using FuncType = bool(*)(const std::string &key, std::any varPtr, const std::string &value);
            FuncType converterSetter;
            VarHolder(std::any varPtr, FuncType converterSetter = nullptr) : varPtr(std::move(varPtr)), converterSetter(converterSetter) { } // NOLINT(google-explicit-constructor) want to able to easily instantiate
        };

        [[nodiscard]] inline static bool setStringDataHelperStringSetter(std::any &varPtr, const std::string &value) {
            std::string &var = *std::any_cast<std::string*>(varPtr);
            if(var != value) {
                var = value;
                return true;
            }
            return false;
        }

//#if defined(ARDUINO)
//        /* Helper method to generically set string value from a map of key/String value ptr pairs, returns empty string if value not changed.
//         *   `_dataMutex` may be locked already when calling this method if `noLock` is true. */
//        // <overloads>
//        [[nodiscard]] std::string getStringDataHelper(const std::map<const std::string, const String&> &keyVarPairs,
//                                                      const std::string &key, const bool noLock) const {
//            return getStringDataHelper<String>(keyVarPairs, key, noLock, String{HasData::EMPTY_VALUE}).c_str();
//        }
//#endif

        /* Helper method to generically set string value from a map of key/value ptr pairs, returns empty string if value not changed.
         *   `_dataMutex` may be locked already when calling this method if `noLock` is true. 
         *   `converterSetter` is optional and can be used to convert the value and set it using varPtr.*/
        [[nodiscard]] std::string setStringDataHelper(std::map<const std::string, VarHolder> keyVarHolderPairs_,
                                                     const std::string &key, const std::string &value, const bool noLock) {
            const auto _readOnlyKeys = getReadOnlyKeys();
            if(std::find(_readOnlyKeys.begin(), _readOnlyKeys.end(), key) != _readOnlyKeys.end()) {
                Logger::logw(getTag(), "Key '%s' is read-only", key.c_str());
                return "";
            }
            auto keyVarHolderPairs = std::move(keyVarHolderPairs_);
            if(keyVarHolderPairs.find(key) == keyVarHolderPairs.end()) {
                Logger::logv(getTag(), "Invalid key '%s', not found", key.c_str());
                return "";
            }

            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
                l.lock();
            VarHolder &varHolder = keyVarHolderPairs.at(key);
            if(varHolder.converterSetter != nullptr) {
                if(!varHolder.converterSetter(key, varHolder.varPtr, value)) {
                    Logger::logw(getTag(), "Same or invalid value or conversion failed for value '%s' for key '%s'", value.c_str(), key.c_str());
                    return "";
                }
                return key;
            }
            return setStringDataHelperStringSetter(varHolder.varPtr, value) ? key : "";
        }

};


#endif // HAS_DATA_H_