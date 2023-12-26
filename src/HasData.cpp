#include <map>
#include <string>
#include <Preferences.h>
#include "CoopLogger.h"
#include "HasData.h"


// load data from the NVS partition in namespace provided from getNVSNS() into keys provided from getNVSIDs()
bool HasData::loadNvsData() {
    const auto nvsNamespace = getNvsNamespace();
    CoopLogger::logv(TAG, "[loadNVSData] for %s", nvsNamespace.c_str());

    if(getNvsKeys().empty()) {
        CoopLogger::logw(TAG, "No NVS IDs defined for %s", nvsNamespace.c_str());
        return false;
    }
    Preferences nvs;
    std::scoped_lock l{nvsDataMutex, _dataMutex};

    if(!nvs.begin(nvsNamespace.c_str(), true)) {
        if(!nvs.begin(nvsNamespace.c_str(), false)) {
            CoopLogger::loge(TAG, "Failed to open NVS (rw) for init namespace %s", nvsNamespace.c_str());
            return false;
        }
        nvs.end();
        if(!nvs.begin(nvsNamespace.c_str(), true)) {
            CoopLogger::loge(TAG, "Failed to open NVS (ro) after successful init namespace %s", nvsNamespace.c_str());
            return false;
        }
    }

    for(const auto &key : getNvsKeys()) {
        const auto &readOnlyKeys = getReadOnlyKeys();
        if(nvs.isKey(key.c_str()) && std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) == readOnlyKeys.end()) {
            const auto value = nvs.getString(key.c_str()).c_str();
            const bool wasUpdated = set(key, value);
            if(!wasUpdated)
                CoopLogger::loge(TAG, "Failed to load %s = %s", key.c_str(), value);
            else
                CoopLogger::logv(TAG, "Loaded %s = %s", key.c_str(), value);
        }
    }

    nvs.end();
    return true;
}

// save data to the NVS partition in namespace provided from getNVSNS() into keys provided from keys param
bool HasData::saveNvsData(const std::vector<std::string> &keys, const bool _dataAlreadyLocked) const {
    const auto nvsNamespace = getNvsNamespace();
    CoopLogger::logv(TAG, "[saveNVSData] for %s", nvsNamespace.c_str());

    if(nvsNamespace.empty()) {
        CoopLogger::logw(TAG, "No NVS IDs defined for %s", nvsNamespace.c_str());
        return false;
    }
    if(keys.empty()) {
        CoopLogger::logw(TAG, "No keys sent to save");
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
        CoopLogger::loge(TAG, "Failed to open (rw) NVS namespace %s", nvsNamespace.c_str());
        return false;
    }

    const auto &readOnlyKeys = getReadOnlyKeys();
    const auto &nvsKeys = getNvsKeys();
    for(const auto &key : keys) {
        if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) == readOnlyKeys.end() &&
           std::find(nvsKeys.begin(), nvsKeys.end(), key) != nvsKeys.end()) {
            const auto value = get(key);
            if(value == EMPTY_VALUE || !nvs.putString(key.c_str(), value.c_str())) {
                CoopLogger::loge(TAG, "Failed to save %s = %s", key.c_str(), value.c_str());
                nvs.end();
                return false;
            }
            CoopLogger::logv(TAG, "Saved %s = %s", key.c_str(), value.c_str());
        }
    }

    nvs.end();
    return true;
}

bool HasData::deleteNvsData() const {
    const auto nvsNamespace = getNvsNamespace();
    CoopLogger::logv(TAG, "[deleteNVSData] for %s", nvsNamespace.c_str());

    Preferences nvs;
    std::scoped_lock l{nvsDataMutex};

    if(!nvs.begin(nvsNamespace.c_str(), false)) {
        CoopLogger::loge(TAG, "Failed to open (rw) NVS namespace %s", nvsNamespace.c_str());
        return false;
    }

    if(!nvs.clear()) {
        CoopLogger::loge(TAG, "Failed to clear NVS namespace %s", nvsNamespace.c_str());
        nvs.end();
        return false;
    }

    nvs.end();

    return true;
}

std::map<std::string, std::string> HasData::getData() const {
    const auto keys = getKeys();
    std::map<std::string, std::string> data;
    std::scoped_lock l{_dataMutex};
    for(auto &key : keys) {
        data.emplace(key, getWithOptLock(key, true));
    }
    return data;
}

bool HasData::setData(const std::map<std::string, std::string> &newData) {
    std::vector<std::string> changedKeys;
    const auto &readOnlyKeys = getReadOnlyKeys();
    std::scoped_lock l{_dataMutex};
    for(const auto &key : getKeys()) {
        if (std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) == readOnlyKeys.end()) {
            const auto &value = newData.at(key);
            if(setWithOptLockAndUpdate(key, value, true, false)) {
                changedKeys.emplace_back(key);
            } else {
                CoopLogger::loge(TAG, "Failed to save %s = %s", key.c_str(), value.c_str());
            }
        }
    }
    const auto changed = changedKeys.empty();
    if(changed) {
        return updateObj(changedKeys, true);
    }
    return changed;
}