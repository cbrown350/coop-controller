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
#include <algorithm>
#include <cmath>

// #define XC_GPIO_DEBUG_OUT RED_LED_OUT_B
// #define TIMER_GPIO_DEBUG_OUT GREEN_LED_OUT_B

class LeadingEdgePhaseDimmer : public HasData<> {
    public:
        inline static constexpr const char *TAG{"LEPhD"};
        inline static constexpr DRAM_ATTR unsigned VALID_FREQUENCIES_HZ[]{60, 50};
        inline static constexpr DRAM_ATTR unsigned minFreq = *std::min_element(std::begin(VALID_FREQUENCIES_HZ), std::end(VALID_FREQUENCIES_HZ));
        inline static constexpr DRAM_ATTR unsigned maxFreq = *std::max_element(std::begin(VALID_FREQUENCIES_HZ), std::end(VALID_FREQUENCIES_HZ));
        inline static constexpr DRAM_ATTR uint16_t RESOLUTION_MAX_SIZE{1024};
        explicit LeadingEdgePhaseDimmer(const std::string &instanceID, uint8_t triacPin, bool triacPinPolarityInverted = true, uint16_t resolution = 128);
        ~LeadingEdgePhaseDimmer() override;

        [[nodiscard]] const char * getTag() const override { return TAG; }
        void setOn() { setBrightness(_resolution); }
        void setOff() { setBrightness(0); }
        void setMinBrightness() { setBrightness(_minNoFlickerBrightness); }
        void setBrightness(const uint16_t brightness, bool noLock = false) {
            Logger::logv(TAG, "[setBrightness 1] Setting brightness to %d", brightness);
            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
               l.lock();
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
        }
        void setBrightness(const uint16_t brightness, const uint16_t resolution, const uint16_t minNoFlickerBrightness = 0, bool noLock = false) {
            Logger::logv(TAG, "[setBrightness 2] Setting brightness to %d", brightness);
            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
               l.lock();
            _brightness = 0; // prevent out of range brightness temporarily
            setResolution(resolution, true);
            setMinNoFlickerBrightness(minNoFlickerBrightness, true);
            setBrightness(brightness, true);
        }
        void setPercentApparentBrightness(const uint8_t percent, bool noLock = false) {
            Logger::logv(TAG, "[setPercentBrightness] Setting percent brightness to %d", percent);
            if(percent > 100) {
                Logger::loge(TAG, "Invalid percent, %d is greater than 100", percent);
                return;
            }
            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
               l.lock();
            const double percHalfPeriod = std::acos(1-(double)2*percent/100)/PI;
            setBrightness(percHalfPeriod *_resolution, true);
        }
        void setMinNoFlickerBrightness(const uint16_t minNoFlickerBrightness, bool noLock = false) {
            Logger::logv(TAG, "[setMinNoFlickerBrightness] Setting minNoFlickerBrightness to %d", minNoFlickerBrightness);
            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
               l.lock();
            if(minNoFlickerBrightness >= _resolution) {
                Logger::loge(TAG, "minNoFlickerBrightness %d is greater than resolution %d, setting to 1/3", minNoFlickerBrightness, _resolution/1);
                this->_minNoFlickerBrightness = _resolution/3;
                return;
            }
            Logger::logv(TAG, "Setting minNoFlickerBrightness to %d", minNoFlickerBrightness);
            this->_minNoFlickerBrightness = minNoFlickerBrightness;
            setBrightness(_brightness, true);
        }

        void setResolution(const uint16_t resolution, bool noLock = false) {
            Logger::logv(TAG, "[setResolution] Setting resolution to %d", resolution);
            if(resolution > RESOLUTION_MAX_SIZE || resolution < 2) {
                Logger::loge(TAG, "resolution %d is out of range (min 2 to max %d)", resolution, RESOLUTION_MAX_SIZE);
                return;
            }
            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
               l.lock();               
            _brightness = 0; // prevent out of range brightness temporarily
            const auto newNoFlickerBrightess = _minNoFlickerBrightness * resolution / _resolution;
            Logger::logv(TAG, "Adjusting no-flicker brightness to %d for new resolution", newNoFlickerBrightess);
            _minNoFlickerBrightness = newNoFlickerBrightess;
            const auto newBrightness = _brightness * resolution / _resolution;
            Logger::logv(TAG, "Setting resolution to %d (and brightness to %d)", resolution, newBrightness);
            _resolution = resolution;
            setBrightness(newBrightness, true);
        }

        [[nodiscard]] uint16_t getResolution(bool noLock = false) const {
            Logger::logv(TAG, "[getResolution] Getting resolution");
//            std::unique_lock l{_dataMutex, std::defer_lock};
//            if(!noLock)
//                l.lock();
            return _resolution; // is atomic, no lock needed
        }
        [[nodiscard]] uint16_t getBrightness(bool noLock = false) const {
            Logger::logv(TAG, "[getBrightness] Getting brightness");
//            std::unique_lock l{_dataMutex, std::defer_lock};
//            if(!noLock)
//                l.lock();
            return _brightness; // is atomic, no lock needed
        }
        [[nodiscard]] uint8_t getPercentApparentBrightness(bool noLock = false) const {
            Logger::logv(TAG, "[getPercentBrightness] Getting percentbrightness");
            std::unique_lock l{_dataMutex, std::defer_lock};
            if(!noLock)
               l.lock();
            return std::round((0.5 - std::cos(((double)getBrightness(true)/_resolution)*PI)/2)*100);
        }
        [[nodiscard]] uint16_t getMinNoFlickerBrightness(bool noLock = false) const {
            Logger::logv(TAG, "[getMinNoFlickerBrightness] Getting minNoFlickerBrightness");
//            std::unique_lock l{_dataMutex, std::defer_lock};
//            if(!noLock)
//                l.lock();
            return _minNoFlickerBrightness; // is atomic, no lock needed
        }

        static unsigned getHalfPeriodUs() { return halfPeriod_us; }

        // HasData
        inline static constexpr const char *RESOLUTION{"resolution"};
        inline static constexpr const char *BRIGHTNESS{"brightness"};
        inline static constexpr const char *PERCENT_APPARENT_BRIGHTNESS{"percent_apparent_brightness"};
        inline static constexpr const char *FREQUENCY{"frequency"};
        inline static const std::vector<std::string> readOnlyKeys{FREQUENCY};
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, {RESOLUTION, BRIGHTNESS, PERCENT_APPARENT_BRIGHTNESS}, true);


        using HasData::getData;
        using HasData::setData;
        using HasData::get;
        using HasData::set;
        [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
        [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }


    private:
        // static vars
        inline static constexpr DRAM_ATTR unsigned HYSTERESIS_OFFSET_US{10};
        inline static constexpr DRAM_ATTR unsigned XC_HALF_PULSE_WIDTH_US{140};
        inline static constexpr DRAM_ATTR double freqMargin{.1};

        inline static unsigned halfPeriod_us_runningAvg[5];
        inline static std::atomic<unsigned> halfPeriod_us{8333};
        inline static bool maskXC = false;
        inline static unsigned long xCTime = 0;
        inline static unsigned long lastXCTime = 0;

        inline static hw_timer_t *Timer0_Cfg{nullptr};

        inline static std::mutex instancesMutex{};
        inline static std::vector<LeadingEdgePhaseDimmer*> instances = {};

        // instance vars
        const uint8_t _triacPin;
        const bool _triacPinPolarityInverted;
        std::atomic<uint16_t> _resolution{128};
        std::atomic<uint16_t> _brightness{0};
        std::atomic<uint16_t> _minNoFlickerBrightness{128/3};


        // methods
        static void init();
        static void init(void*);
        static void deinit();
        static void zeroXPulseISR(void*);
        static unsigned caclHalfPeriodUSRunningAvg(const unsigned newVal);
        static unsigned calcHalfPeriod_us(const unsigned elapsed_us);
        static void timerISR();
        
        inline static constexpr int IRAM_ATTR abs(int x) {
            return x < 0 ? -x : x;
        }

        inline static constexpr unsigned IRAM_ATTR freqToHalfPeriod_us(const unsigned freq) {
            if(freq == 0)
                return freqToHalfPeriod_us(minFreq);
            return (1000000/freq) / 2;
        }
        // alarm div TIMER_TICK_US = 1/TIMER_TICK_US Hz, TIMER_TICK_US period
        inline static const DRAM_ATTR uint64_t TIMER_TICK_US{freqToHalfPeriod_us(maxFreq)/RESOLUTION_MAX_SIZE-1};

        inline static constexpr unsigned IRAM_ATTR halfPeriod_us_toFreq(const unsigned _halfPeriod_us) {
            if(_halfPeriod_us == 0) 
                return maxFreq;
            return 1000000 / (_halfPeriod_us * 2);
        }

        void toggleTriacPin(unsigned currCyclePercentage);

        // HasData
        using HasData::getNvsKeys;
        using HasData::getNvsNamespace;
        using HasData::loadNvsData;
        using HasData::saveNvsData;
        using HasData::deleteNvsData;
        using HasData::updateObj;
        [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
            Logger::logv(TAG, "[getWithOptLock] Getting %s", key.c_str());
//            std::unique_lock l{_dataMutex, std::defer_lock};
//            switch(utils::hashstr(key.c_str())) {
//                case utils::hashstr(BRIGHTNESS): {
//                    if(!noLock)
//                        l.lock();
//                    return std::to_string(getBrightness(true));
//                }
//                case utils::hashstr(RESOLUTION): {
//                    if(!noLock)
//                        l.lock();
//                    return std::to_string(getResolution(true));
//                }
//                case utils::hashstr(FREQUENCY): {
//                    if(!noLock)
//                        l.lock();
//                    return std::to_string(halfPeriod_us > 0 ? 1000000 / (2 * halfPeriod_us) : 0);
//                }
//                default: {}
//            }
//            Logger::logw(TAG, "Invalid key %s", key.c_str());
//            return HasData::EMPTY_VALUE;
//            std::unique_lock l(_dataMutex, std::defer_lock);
//            if(!noLock)
//                l.lock();
//            return getStringDataHelper({
//                {BRIGHTNESS, std::to_string(getBrightness(true))},
//                {RESOLUTION, std::to_string(getResolution(true))},
//                {FREQUENCY, std::to_string(halfPeriod_us > 0 ? 1000000 / (2 * halfPeriod_us) : 0)}
//            }, key, noLock || noLock);
            using retType = std::string;
            using FunPtrType = retType(*)(const LeadingEdgePhaseDimmer*, const bool noLock);
            return getStringDataHelper<FunPtrType>({
                {BRIGHTNESS, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getBrightness(noLock)); }},
                {PERCENT_APPARENT_BRIGHTNESS, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getPercentApparentBrightness(noLock)); }},
                {RESOLUTION, (FunPtrType)[](const auto l, const bool noLock) -> retType { return std::to_string(l->getResolution(noLock)); }},
                {FREQUENCY, (FunPtrType)[](const auto, const bool noLock) -> retType { return std::to_string(halfPeriod_us_toFreq(LeadingEdgePhaseDimmer::halfPeriod_us)); }}  // is atomic, no lock needed
            }, key, true, (FunPtrType)[](const auto, const bool) -> retType { return HasData::EMPTY_VALUE; })(this, noLock);
        }


        [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, const bool noLock, const bool doObjUpdate) override {
            if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end() ||
                std::find(keys.begin(), keys.end(), key) == keys.end()) {
                Logger::logw(TAG, "Key %s is read-only or not found", key.c_str());
                return false;
            }
            Logger::logv(TAG, "[setWithOptLockAndUpdate]Setting %s to %s", key.c_str(), value.c_str());

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
                    const unsigned val = std::stof(value);
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

//            using retType = std::string;
//            using FunPtrType = retType(*)(LeadingEdgePhaseDimmer*, const std::atomic<uint16_t>&, std::unique_lock<std::mutex> &);
//            const auto updatedKey = getStringDataHelper<FunPtrType>({
//                  {BRIGHTNESS, (FunPtrType)[](auto i, const auto &v, auto &l) -> retType {
//                                                        l.lock(); need to check noLock here
//                                                        if(i->getBrightness(true) != v) {
//                                                            i->setBrightness(v, true);
//                                                            return BRIGHTNESS;
//                                                        }
//                                                        return "";
//                  }},
//                  {RESOLUTION, (FunPtrType)[](auto i, const auto &v, auto &l) -> retType {
//                                                          l.lock(); need to check noLock here
//                                                          if(i->getResolution(true) != v) {
//                                                              i->setResolution(v, true);
//                                                              return RESOLUTION;
//                                                          }
//                                                          return "";
//                  }},
//               }, key, noLock || l.owns_lock(), (FunPtrType)[](auto, const auto&, auto&) -> retType { return ""; })
//                       (this, std::stoi(value), l);
//
////            const auto updatedKey = setStringDataHelper<std::atomic<uint16_t>>({
////                                    {BRIGHTNESS, _brightness},
////                                    {RESOLUTION, _resolution}
////                                }, key, std::stoi(value), noLock || l.owns_lock());
//            bool updated = !updatedKey.empty();
//            if(updated) {
//                if (doObjUpdate) {
//                    keysToUpdateOnObj.push_back(updatedKey);
//                }
//            }


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

