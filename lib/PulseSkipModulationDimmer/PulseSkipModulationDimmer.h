#ifndef PULSE_SKIP_MODULATION_DIMMER_CONTROLLER_H
#define PULSE_SKIP_MODULATION_DIMMER_CONTROLLER_H

#include <ZeroCrossing.h>

class PulseSkipModulationDimmer : public ZeroCrossing {
    public:
        inline static constexpr const char *TAG{"PSMD"};

        inline static constexpr DRAM_ATTR uint16_t max_cycles{128};

        explicit PulseSkipModulationDimmer(const std::string &instanceID, uint8_t triacPin, bool triacPinPolarityInverted = true, uint16_t cycles = 128);
        ~PulseSkipModulationDimmer() override;

        [[nodiscard]] const char * getTag() const override { return TAG; }
        // Get tags for all classes in hierarchy
        [[nodiscard]] static std::vector<std::string> getTags() { return {TAG, ZeroCrossing::TAG}; }

        void setOn() { setBrightness(_cycles); }
        void setOff() { setBrightness(0); }
        void setBrightness(uint16_t brightness, bool noLock = false);
        void setBrightness(uint16_t brightness, uint16_t cycles, bool noLock = false);
        void setCycles(uint16_t cycles, bool noLock = false);
        [[nodiscard]] uint16_t getBrightness(bool noLock = false) const;
        [[nodiscard]] uint16_t getCycles(bool noLock = false) const;

        // HasData
        inline static constexpr const char *MAX_CYCLES{"max_cycles"};
        inline static constexpr const char *CYCLES{"cycles"}; // cycles over which the brightness is spread
        inline static constexpr const char *BRIGHTNESS{"brightness"};
        inline static const std::vector<std::string> readOnlyKeys = utils::concat({MAX_CYCLES}, ZeroCrossing::readOnlyKeys, true);
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys,
                                    utils::concat(ZeroCrossing::keys, {CYCLES, BRIGHTNESS}, true), true);

        [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
        [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }


private:
    // instance vars
    const uint8_t _triacPin;
    const bool _triacPinPolarityInverted;
    uint16_t _cycles{128};
    uint16_t _brightness{0};

    bool currHalfCycleCalcDone{false};
    unsigned halfCycleCount{0};
    unsigned cyclesFiredCount{0};

    /* Calculate if triac should fire on this cycle depending on brightness.
        Brightness is the number of cycles for which the triac should be on compared to the total number of cycles.
        On cycles should be spread out across the total number of cycles.
     */
    static void IRAM_ATTR timerISRCall(ZeroCrossing* instance, unsigned currCyclePercentage);
    void addInstanceTimerISR() override {
        InstanceISRFunc timerISR = &PulseSkipModulationDimmer::timerISRCall;
        instancesISRs.emplace_back(this, timerISR);
    }

    // HasData
    [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
        Logger::logv(TAG, "[getWithOptLock] Getting %s", key.c_str());

        using retType = std::string;
        using FunPtrType = retType(*)(const PulseSkipModulationDimmer*, const bool noLock);
        std::string thisData = getStringDataHelper<FunPtrType>({
                       {BRIGHTNESS, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getBrightness(noLock)); }},
                       {CYCLES, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getCycles(noLock)); }},
                       {MAX_CYCLES, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(max_cycles); }}
               }, key, true, (FunPtrType)[](const auto, const bool) -> retType { return HasData::EMPTY_VALUE; })(this, noLock);

        if(!thisData.empty() && thisData != HasData::EMPTY_VALUE)
            return thisData;
        return ZeroCrossing::getWithOptLock(key, noLock);
    }

    [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value_raw, const bool noLock, const bool doObjUpdate) override {
        if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
           std::find(keys.begin(), keys.end(), key) == keys.end()) {
            Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
            return false;
        }
        const std::string value = utils::trim_clean(value_raw);
        Logger::logv(TAG, "[setWithOptLockAndUpdate] Setting %s to %s", key.c_str(), value.c_str());

        const bool zeroCrossingUpdated = ZeroCrossing::setWithOptLockAndUpdate(key, value, noLock, doObjUpdate);
        if(zeroCrossingUpdated)
            return true;

        static std::vector<std::string> keysToUpdateOnObj = {};
        bool updated;

        std::unique_lock l{_dataMutex, std::defer_lock};
        switch(utils::hashstr(key.c_str())) {
            case utils::hashstr(BRIGHTNESS): {
                if(!utils::isPositiveNumber(value)) {
                    Logger::loge(TAG, "Invalid BRIGHTNESS value %s", value.c_str());
                    return false;
                }
                const uint16_t val = std::stoi(value);
                if(!noLock)
                    l.lock();
                updated = val != getBrightness(true);
                if(updated) {
                    setBrightness(val, true);
                    if (doObjUpdate)
                        keysToUpdateOnObj.push_back(key);
                }
                break;
            }
            case utils::hashstr(CYCLES): {
                if(!utils::isPositiveNumber(value)) {
                    Logger::loge(TAG, "Invalid RESOLUTION value %s", value.c_str());
                    return false;
                }
                const uint16_t val = std::stoi(value);
                if(!noLock)
                    l.lock();
                updated = val != getCycles(true);
                if(updated) {
                    setCycles(val, true);
                    if (doObjUpdate)
                        keysToUpdateOnObj.push_back(key);
                }
                break;
            }
            default: {
                Logger::logw(TAG, "Invalid key %s", key.c_str());
                return false;
            }
        }

        if(doObjUpdate && !keysToUpdateOnObj.empty()) {
            const bool objUpdated = updateObj(keysToUpdateOnObj, noLock || l.owns_lock()) || updated; // include updated since vars were updated
            if(objUpdated) {
                Logger::logv(TAG, "Updated %s", utils::join(keysToUpdateOnObj, ", ").c_str());
                keysToUpdateOnObj.clear();
            }
            return objUpdated;
        }
        return updated;
    }
};



#endif // PULSE_SKIP_MODULATION_DIMMER_CONTROLLER_H