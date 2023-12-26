#ifndef LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H
#define LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H

#include <esp_attr.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
//#include <esp32-hal-timer.h>
#include <vector>
#include "CoopLogger.h"
#include "HasData.h"

class LeadingEdgePhaseDimmer : public HasData {
    public:
        static constexpr const char *TAG{"LEPhD"};
        static constexpr uint16_t RESOLUTION_MAX_SIZE{2048};
        static constexpr uint64_t TIMER_TICK_US{6}; // alarm div 10 = 100kHz, 10us period
        explicit LeadingEdgePhaseDimmer(const std::string &instanceID, uint8_t triacPin, bool triacPinPolarityInverted = true, uint16_t resolution = 128);
        ~LeadingEdgePhaseDimmer() override;

        void setBrightness(const uint16_t brightness) {
            uint16_t tmpResolution = _resolution;
            if(brightness > tmpResolution) {
                CoopLogger::loge(TAG, "brightness %d is greater than resolution %d", brightness, tmpResolution);
                return;
            }
            if(brightness > 0 && brightness < _minNoFlickerBrightness) {
                this->_brightness = _minNoFlickerBrightness/1;
            } else {
                this->_brightness = brightness;
            }
//            CoopLogger::logv(TAG, "Setting brightness to %d", _brightness/1);
        }
        void setBrightness(const uint16_t brightness, const uint16_t resolution, const uint16_t minNoFlickerBrightness = RESOLUTION_MAX_SIZE) {
            setResolution(resolution);
            setMinNoFlickerBrightness(minNoFlickerBrightness);
            setBrightness(brightness);
        }

        void setMinNoFlickerBrightness(const uint16_t minNoFlickerBrightness) {
            if(minNoFlickerBrightness >= _resolution) {
                CoopLogger::loge(TAG, "minNoFlickerBrightness %d is greater than resolution %d, setting to 1/3", minNoFlickerBrightness, _resolution/1);
                this->_minNoFlickerBrightness = _resolution/3;
                return;
            }
            CoopLogger::logv(TAG, "Setting minNoFlickerBrightness to %d", minNoFlickerBrightness);
            this->_minNoFlickerBrightness = minNoFlickerBrightness;
            setBrightness(_brightness);
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
        [[nodiscard]] std::map<std::string, std::string> getData() override {
            return {
                {RESOLUTION, get(RESOLUTION)},
                {BRIGHTNESS, get(BRIGHTNESS)},
                {FREQUENCY, get(FREQUENCY)}
            };
        }
        [[nodiscard]] std::string get(const std::string &key) override {
            if(key == RESOLUTION)
                return std::to_string(_resolution);
            if(key == BRIGHTNESS)
                return std::to_string(_brightness);
            if(key == FREQUENCY)
                return std::to_string(halfPeriod_us > 0 ? 1000000 / (2 * halfPeriod_us) : 0);
            CoopLogger::logw(TAG, "Invalid key %s", key.c_str());
            return "";
        }
        void set(const std::string &key, const std::string &value) override {
            if(key == RESOLUTION) {
                setResolution(std::stoi(value));
                return;
            }
            if(key == BRIGHTNESS) {
                setBrightness(std::stoi(value));
                return;
            }
            CoopLogger::logw(TAG, "Invalid key %s", key.c_str());
        }
        void setData(const std::map<std::string, std::string> &data) override {
            for(auto &pair : data)
                set(pair.first, pair.second);
        }


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
        void setResolution(const uint16_t resolution) {
            if(resolution > RESOLUTION_MAX_SIZE || resolution < 2) {
                CoopLogger::loge(TAG, "resolution %d is out of range (min 2 to max %d)", resolution, RESOLUTION_MAX_SIZE);
                return;
            }
            CoopLogger::logv(TAG, "Setting resolution to %d", resolution);
            this->_resolution = resolution;
        }
        void toggleTriacPin(unsigned currPercentage);

        // HasData
        using HasData::getNvsKeys;
        using HasData::getNvsNamespace;
        using HasData::loadNvsData;
        using HasData::saveNvsData;
        using HasData::deleteNvsData;
};


#endif // LEADING_EDGE_PHASE_DIMMER_CONTROLLER_H

