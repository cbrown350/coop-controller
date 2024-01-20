#ifndef LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H
#define LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H

#include "settings.h"
#include "ZeroCrossing.h"

#include <Logger.h>
#include <HasData.h>
#include <ZeroCrossing.h>
#include <utils.h>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>
#include <cmath>

class LeadingEdgePhaseDimmer : public ZeroCrossing {
    public:
        inline static constexpr const char *TAG{"LEPhD"};
        explicit LeadingEdgePhaseDimmer(const std::string &instanceID, uint8_t triacPin, bool triacPinPolarityInverted = true, uint16_t resolution = 128);
        ~LeadingEdgePhaseDimmer() override;

        [[nodiscard]] const char * getTag() const override { return TAG; }
        // Get tags for all classes in hierarchy
        [[nodiscard]] static std::vector<std::string> getTags() { return {TAG, ZeroCrossing::TAG}; }
        void setOn() { setBrightness(_resolution); }
        void setOff() { setBrightness(0); }
        void setMinBrightness() { setBrightness(_minNoFlickerBrightness); }

        void setBrightness(uint16_t brightness, bool noLock = false);
        void setBrightness(uint16_t brightness, uint16_t resolution, uint16_t minNoFlickerBrightness = 0, bool noLock = false);
        void setPercentApparentBrightness(uint8_t percent, bool noLock = false);
        void setMinNoFlickerBrightness(uint16_t minNoFlickerBrightness, bool noLock = false);
        void setResolution(uint16_t resolution, bool noLock = false);
        [[nodiscard]] uint16_t getResolution(bool noLock = false) const;
        [[nodiscard]] uint16_t getBrightness(bool noLock = false) const;
        [[nodiscard]] uint8_t getPercentApparentBrightness(bool noLock = false) const;
        [[nodiscard]] uint16_t getMinNoFlickerBrightness(bool noLock = false) const;

        // HasData
        inline static constexpr const char *RESOLUTION{"resolution"};
        inline static constexpr const char *BRIGHTNESS{"brightness"};
        inline static constexpr const char *PERCENT_APPARENT_BRIGHTNESS{"percent_apparent_brightness"};
        inline static const std::vector<std::string> readOnlyKeys = utils::concat({}, ZeroCrossing::readOnlyKeys, true);
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys,
                                                               utils::concat(ZeroCrossing::keys, {RESOLUTION, BRIGHTNESS, PERCENT_APPARENT_BRIGHTNESS}, true), true);

        [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
        [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }


    private:
        // instance vars
        const uint8_t _triacPin;
        const bool _triacPinPolarityInverted;
        uint16_t _resolution{128};
        uint16_t _brightness{0};
        uint16_t _minNoFlickerBrightness{128/3};

        static void IRAM_ATTR timerISRCall(ZeroCrossing* instance, unsigned currCyclePercentage);
        void addInstanceTimerISR() override {
            InstanceISRFunc timerISR = &LeadingEdgePhaseDimmer::timerISRCall;
            instancesISRs.emplace_back(this, timerISR);
        }

    // HasData
        [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
            Logger::logv(TAG, "[getWithOptLock] Getting %s", key.c_str());

            using retType = std::string;
            using FunPtrType = retType(*)(const LeadingEdgePhaseDimmer*, const bool noLock);
            std::string thisData = getStringDataHelper<FunPtrType>({
                {BRIGHTNESS, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getBrightness(noLock)); }},
                {PERCENT_APPARENT_BRIGHTNESS, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getPercentApparentBrightness(noLock)); }},
                {RESOLUTION, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getResolution(noLock)); }}
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
                case utils::hashstr(PERCENT_APPARENT_BRIGHTNESS): {
                    if(!utils::isPositiveNumber(value)) {
                        Logger::loge(TAG, "Invalid PERCENT_BRIGHTNESS value %s", value.c_str());
                        return false;
                    }
                    const auto val = (unsigned)std::stof(value);
                    if(val > 100) {
                        Logger::loge(TAG, "Invalid PERCENT_BRIGHTNESS value %s", value.c_str());
                        return false;
                    }
                    if(!noLock)
                        l.lock();
                    updated = val != getPercentApparentBrightness(true);
                    if(updated) {
                        setPercentApparentBrightness(val, true);
                        if (doObjUpdate)
                            keysToUpdateOnObj.push_back(key);
                    }
                    break;
                }
                case utils::hashstr(RESOLUTION): {
                    if(!utils::isPositiveNumber(value)) {
                        Logger::loge(TAG, "Invalid RESOLUTION value %s", value.c_str());
                        return false;
                    }
                    const uint16_t val = std::stoi(value);
                    if(!noLock)
                        l.lock();
                    updated = val != getResolution(true);
                    if(updated) {
                        setResolution(val, true);
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


#endif // LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H

