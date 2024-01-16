#include "HasData.h"

#include <Preferences.h>

#include <Logger.h>

#include <map>
#include <string>
#include <algorithm>

/*
 * This is a template class for HasData that uses the default Logger L type and Arduino Preferences to save NVS data.
 */

template<>
bool HasData<>::loadNvsData() {
    const auto nvsNamespace = getNvsNamespace();
    Logger::logv(getTag(), "[loadNVSData] for namespace %s", nvsNamespace.c_str());

    if(getNvsKeys().empty()) {
        Logger::logw(getTag(), "No NVS IDs defined for namespace %s", nvsNamespace.c_str());
        return false;
    }
    Preferences nvs;
    std::scoped_lock l{nvsDataMutex, _dataMutex};

    if(!nvs.begin(nvsNamespace.c_str(), true)) {
        // Try to create namespace
        if(!nvs.begin(nvsNamespace.c_str(), false)) {
            Logger::loge(getTag(), "Failed to open NVS (rw) for init namespace %s", nvsNamespace.c_str());
            return false;
        }
        nvs.end();
        // Try to open namespace again
        if(!nvs.begin(nvsNamespace.c_str(), true)) {
            Logger::loge(getTag(), "Failed to open NVS (ro) after successful init namespace %s", nvsNamespace.c_str());
            return false;
        }
        Logger::logv(getTag(), "Namespace %s was just created, so no data to load", nvsNamespace.c_str());
        nvs.end();
        return true;
    }

//    bool keysUpdated = false;
    for(const auto &key : getNvsKeys()) {
        const auto &readOnlyKeys = getReadOnlyKeys();
        if(nvs.isKey(key.c_str()) &&
                    std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) == readOnlyKeys.end()) {
            const std::string value = nvs.getString(key.c_str(), EMPTY_VALUE).c_str();
            const bool wasUpdated = setWithOptLockAndUpdate(key, value, true, false);
//            keysUpdated |= wasUpdated;
            if(!wasUpdated)
                Logger::loge(getTag(), "Failed to load %s = %s", key.c_str(), value.c_str());
            else
                Logger::logv(getTag(), "Loaded %s = %s", key.c_str(), value.c_str());
        }
    }
    nvs.end();
    // Don't want to reload object automatically since may be recursive loop
    // bool updatedObj = true;
    // if(keysUpdated) 
    //     updatedObj = updateObj(getNvsKeys(), true);
    // if(!updatedObj)
    //     Logger::loge(getTag(), "Failed to update object after loading NVS data");

    // return updatedObj;
    return true;
}

template<>
bool HasData<>::saveNvsData(const std::vector<std::string> &keys, const bool _dataAlreadyLocked) const {
    const auto nvsNamespace = getNvsNamespace();
    Logger::logv(getTag(), "[saveNVSData] for namespace %s", nvsNamespace.c_str());

    const auto &nvsKeys = getNvsKeys();
    if(nvsKeys.empty()) {
        Logger::logw(getTag(), "No NVS IDs defined for namespace %s", nvsNamespace.c_str());
        return false;
    }
    if(keys.empty()) {
        Logger::logw(getTag(), "No keys sent to save");
        return false;
    }

    Preferences nvs;
    std::unique_lock nvsLock{nvsDataMutex, std::defer_lock};
    std::unique_lock _dataLock{_dataMutex, std::defer_lock};
    if(!_dataAlreadyLocked)
        std::lock(nvsLock, _dataLock);
    else
        nvsLock.lock();

    if(!nvs.begin(nvsNamespace.c_str(), false)) {
        Logger::loge(getTag(), "Failed to open (rw) NVS namespace %s", nvsNamespace.c_str());
        return false;
    }

    const auto &readOnlyKeys = getReadOnlyKeys();
    for(const auto &key : keys) {
        if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) == readOnlyKeys.end() &&
                  std::find(nvsKeys.begin(), nvsKeys.end(), key) != nvsKeys.end()) {
            const auto value = getWithOptLock(key, true);
            if(std::string(nvs.getString(key.c_str(), EMPTY_VALUE).c_str()) == value) {
               Logger::logv(getTag(), "Skipping %s = %s, already saved", key.c_str(), value.c_str());
               continue;
            }
            if(value == EMPTY_VALUE) {
                nvs.remove(key.c_str());
            } else if(nvs.putString(key.c_str(), value.c_str()) != value.length()) {
                Logger::loge(getTag(), "Failed to save %s = %s", key.c_str(), value.c_str());
                nvs.end();
                return false;
            }
            Logger::logv(getTag(), "Saved %s = %s", key.c_str(), value.c_str());
        }
    }

    nvs.end();
    return true;
}

template<>
bool HasData<>::deleteNvsData() const {
    const auto nvsNamespace = getNvsNamespace();
    Logger::logv(getTag(), "[deleteNVSData] for namespace %s", nvsNamespace.c_str());

    Preferences nvs;
    std::scoped_lock l{nvsDataMutex};

    if(!nvs.begin(nvsNamespace.c_str(), false)) {
        Logger::loge(getTag(), "Failed to open (rw) NVS namespace %s", nvsNamespace.c_str());
        return false;
    }

    if(!nvs.clear()) {
        Logger::loge(getTag(), "Failed to clear NVS namespace %s", nvsNamespace.c_str());
        nvs.end();
        return false;
    }

    nvs.end();

    return true;
}

template<>
std::map<std::string, std::string> HasData<>::getData() const {
    Logger::logv(getTag(), "[getData] for instanceID %s", instanceID.c_str());
    const auto keys = getKeys();
    std::map<std::string, std::string> data;
    std::scoped_lock l{_dataMutex};
    for(auto &key : keys) {
        data.emplace(key, getWithOptLock(key, true));
    }
    return data;
}

template<>
bool HasData<>::setData(const std::map<std::string, std::string> &newData) {
    Logger::logv(getTag(), "[setData] for instanceID %s", instanceID.c_str());
    std::vector<std::string> changedKeys;
    const auto &readOnlyKeys = getReadOnlyKeys();
    std::scoped_lock l{_dataMutex};
    for(const auto &key : getKeys()) {
        if (std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) == readOnlyKeys.end() &&
                newData.find(key) != newData.end()) {
            const auto &value = newData.at(key);
            if(setWithOptLockAndUpdate(key, value, true, false)) {
                changedKeys.emplace_back(key);
            } else {
                Logger::loge(getTag(), "Failed to save %s = %s", key.c_str(), value.c_str());
            }
        }
    }
    const auto changed = !changedKeys.empty();
    if(changed) {
        return updateObj(changedKeys, true);
    }
    return changed;
}