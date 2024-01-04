#ifndef LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H
#define LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H

#include "settings.h"
#include <esp_attr.h>
//#include <esp32-hal-timer.h>

#include <Logger.h>
#include <HasData.h>
#include <utils.h>

#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>

class LeadingEdgePhaseDimmer : public HasData<> {
    public:
        inline static constexpr const char *TAG{"LEPhD"};
        static constexpr uint16_t RESOLUTION_MAX_SIZE{2048};
        static constexpr uint64_t TIMER_TICK_US{6}; // alarm div 10 = 100kHz, 10us period
        explicit LeadingEdgePhaseDimmer(const std::string &instanceID, uint8_t triacPin, bool triacPinPolarityInverted = true, uint16_t resolution = 128);
        ~LeadingEdgePhaseDimmer() override;

        void setBrightness(const uint16_t brightness) {
            uint16_t tmpResolution = _resolution;
            if(brightness > tmpResolution) {
                Logger::loge(TAG, "brightness %d is greater than resolution %d", brightness, tmpResolution);
                return;
            }
            if(brightness > 0 && brightness < _minNoFlickerBrightness) {
                this->_brightness = _minNoFlickerBrightness/1;
            } else {
                this->_brightness = brightness;
            }
//            Logger::logv(TAG, "Setting brightness to %d", _brightness/1);
        }
        void setBrightness(const uint16_t brightness, const uint16_t resolution, const uint16_t minNoFlickerBrightness = RESOLUTION_MAX_SIZE) {
            _brightness = 0; // prevent out of range brightness temporarily
            setResolution(resolution);
            setMinNoFlickerBrightness(minNoFlickerBrightness);
            setBrightness(brightness);
        }

        void setMinNoFlickerBrightness(const uint16_t minNoFlickerBrightness) {
            if(minNoFlickerBrightness >= _resolution) {
                Logger::loge(TAG, "minNoFlickerBrightness %d is greater than resolution %d, setting to 1/3", minNoFlickerBrightness, _resolution/1);
                this->_minNoFlickerBrightness = _resolution/3;
                return;
            }
            Logger::logv(TAG, "Setting minNoFlickerBrightness to %d", minNoFlickerBrightness);
            this->_minNoFlickerBrightness = minNoFlickerBrightness;
            setBrightness(_brightness);
        }

        void setResolution(const uint16_t resolution) {
            if(resolution > RESOLUTION_MAX_SIZE || resolution < 2) {
                Logger::loge(TAG, "resolution %d is out of range (min 2 to max %d)", resolution, RESOLUTION_MAX_SIZE);
                return;
            }
            _brightness = 0; // prevent out of range brightness temporarily
            auto newBrightness = _brightness * resolution / _resolution;
            Logger::logv(TAG, "Setting resolution to %d (and brightness to %d", resolution, newBrightness);
            _resolution = resolution;
            setBrightness(newBrightness);
        }

        [[nodiscard]] uint16_t getResolution() const { return _resolution; }
        [[nodiscard]] uint16_t getBrightness() const { return _brightness; }
        [[nodiscard]] uint16_t getMinNoFlickerBrightness() const { return _minNoFlickerBrightness; }

//        static std::condition_variable& getZeroXCond() { return zeroXCond; }
//        static std::condition_variable& getTimerTickCond() { return timerTickCond; }

        static unsigned getHalfPeriodUs() { return halfPeriod_us; }
        static unsigned getZeroXHalfPulseWidthUs() { return zeroXHalfPulseWidth_us; }

        // HasData
        inline static constexpr const char *RESOLUTION{"resolution"};
        inline static constexpr const char *BRIGHTNESS{"brightness"};
        inline static constexpr const char *FREQUENCY{"frequency"};
        inline static const std::vector<std::string> readOnlyKeys{FREQUENCY};
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, {RESOLUTION, BRIGHTNESS}, true);


        using HasData::getData;
        using HasData::setData;
        using HasData::get;
        using HasData::set;
        [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
        [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }


    private:
        // static vars
//        inline static std::condition_variable zeroXCond{};
//        inline static std::vector<std::function<void()>> zeroXCallbacks = {};
//        inline static std::condition_variable timerTickCond{};
//        inline static std::vector<std::function<void()>> timerTickCallbacks = {};

        inline static unsigned long lastZeroXFallingEdgeTime_us{0};
        inline static constexpr unsigned HYSTERESIS_OFFSET_US{10};
        inline static std::atomic<unsigned> halfPeriod_us{0};
//    inline static unsigned halfPeriod_us{0};
//        inline static std::atomic<unsigned> periodUs{0};
//    inline static std::atomic<unsigned> zeroXHalfPulseWidth_us{300};
        inline static unsigned zeroXHalfPulseWidth_us{140};
//        inline static std::atomic<unsigned> estimatedZeroXSysTimeUs{0};

        inline static hw_timer_t *Timer0_Cfg{nullptr};
        inline static std::atomic<int> timerTicks{0};
//        inline static int timerTicks{0};

        inline static std::mutex instancesMutex{};
        inline static std::vector<LeadingEdgePhaseDimmer*> instances = {};

        // instance vars
        const uint8_t _triacPin;
        const bool _triacPinPolarityInverted;
        std::atomic<uint16_t> _resolution{128};
        std::atomic<uint16_t> _brightness{0};
        std::atomic<uint16_t> _minNoFlickerBrightness{128/3};
//        uint16_t _resolution{128};
//        uint16_t _brightness{0};
//        uint16_t _minNoFlickerBrightness{128/3};

        // methods
        static void init();
        static void deinit();
        static void zeroXPulseISR();
        static void timerISR();
//        static void resetTimerTicks() { timerTicks = (int) zeroXFallingToZeroOffset_us / TIMER_TICK_US; }
        void toggleTriacPin(unsigned currPercentage);

        // HasData
        using HasData::getNvsKeys;
        using HasData::getNvsNamespace;
        using HasData::loadNvsData;
        using HasData::saveNvsData;
        using HasData::deleteNvsData;
        using HasData::updateObj;
        [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
            std::unique_lock l{_dataMutex, std::defer_lock};
            switch(utils::hashstr(key.c_str())) {
                case utils::hashstr(BRIGHTNESS): {
                    if(!noLock)
                        l.lock();
                    return std::to_string(getBrightness());
                }
                case utils::hashstr(RESOLUTION): {
                    if(!noLock)
                        l.lock();
                    return std::to_string(getResolution());
                }
                case utils::hashstr(FREQUENCY): {
                    if(!noLock)
                        l.lock();
                    return std::to_string(halfPeriod_us > 0 ? 1000000 / (2 * halfPeriod_us) : 0);
                }
                default: {}
            }
            Logger::logw(TAG, "Invalid key %s", key.c_str());
            return HasData::EMPTY_VALUE;
        }


        [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, const bool noLock, const bool doObjUpdate) override {
            if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
                std::find(keys.begin(), keys.end(), key) == keys.end()) {
                Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
                return false;
            }

            static std::vector<std::string> keysToUpdateOnObj = {};
            bool updated = false;

            std::unique_lock l{_dataMutex, std::defer_lock};
            switch(utils::hashstr(key.c_str())) {
                case utils::hashstr(BRIGHTNESS): {
                    if(!noLock)
                        l.lock();
                    setBrightness(std::stoi(value));
                    updated = true;
                    keysToUpdateOnObj.push_back(key);
                    break;
                }
                case utils::hashstr(RESOLUTION): {
                    if(!noLock)
                        l.lock();
                    setResolution(std::stoi(value));
                    updated = true;
                    keysToUpdateOnObj.push_back(key);
                    break;
                }
                default: {}
            }
            if(doObjUpdate && !keysToUpdateOnObj.empty()) {
                const bool objUpdated = updateObj(keysToUpdateOnObj, l.owns_lock());
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

