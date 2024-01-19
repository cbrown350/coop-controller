#ifndef ZERO_CROSSING_H
#define ZERO_CROSSING_H

#include "settings.h"
#include <esp_attr.h>
//#include <esp32-hal-timer.h>

#include <Logger.h>
#include <HasData.h>
#include <utils.h>
#include <utils_emb.h>

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>
#include <cmath>

// #define XC_GPIO_DEBUG_OUT RED_LED_OUT_B
// #define TIMER_GPIO_DEBUG_OUT GREEN_LED_OUT_B

class ZeroCrossing : public HasData<> {
    public:
        inline static constexpr const char *TAG{"ZroXg"};
        inline static constexpr DRAM_ATTR uint16_t RESOLUTION_MAX_SIZE{1024}; // max resolution for subclasses, drives TIMER_TICK_US for ISR
        inline static constexpr DRAM_ATTR unsigned VALID_FREQUENCIES_HZ[]{60, 50};
        inline static constexpr DRAM_ATTR unsigned minFreq = *std::min_element(std::begin(VALID_FREQUENCIES_HZ), std::end(VALID_FREQUENCIES_HZ));
        inline static constexpr DRAM_ATTR unsigned maxFreq = *std::max_element(std::begin(VALID_FREQUENCIES_HZ), std::end(VALID_FREQUENCIES_HZ));

        explicit ZeroCrossing(const std::string &instanceID);
        ~ZeroCrossing() override;

        [[nodiscard]] const char * getTag() const override { return TAG; }

protected:
        static constexpr unsigned IRAM_ATTR halfPeriod_us_toFreq(const unsigned _halfPeriod_us) { // definition in header to ensure IRAM_ATTR
            if(_halfPeriod_us == 0)
                return maxFreq;
            return 1000000 / (_halfPeriod_us * 2);
        }

public:
        static unsigned getFrequency() { return halfPeriod_us_toFreq(halfPeriod_us); }

        // HasData
        inline static constexpr const char *FREQUENCY{"frequency"};
        inline static const std::vector<std::string> readOnlyKeys{FREQUENCY};
        inline static const std::vector<std::string> keys = utils::concat(readOnlyKeys, {}, true);

        [[nodiscard]] std::vector<std::string> getKeys() const override { return keys; }
        [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return readOnlyKeys; }


    protected:
        // static vars
        inline static constexpr DRAM_ATTR unsigned HYSTERESIS_OFFSET_US{10}; // for XC pulse, empirical based on circuit
        inline static constexpr DRAM_ATTR unsigned XC_HALF_PULSE_WIDTH_US{140}; // empirical based on circuit
        inline static constexpr DRAM_ATTR double freqMargin{.1};

        inline static unsigned halfPeriod_us{8333};
        inline static bool maskXC = false;
        inline static unsigned long xCTime{0};
        inline static unsigned long lastXCTime{0};

        inline static hw_timer_t *Timer0_Cfg{nullptr};

        inline static std::mutex instancesMutex{};

        // Can't use virtual functions in IRAM, must save them in init() for instances
        virtual void addInstanceTimerISR() = 0;
        using InstanceISRFunc = void(*)(ZeroCrossing* instance, const unsigned currCyclePercentage);
        inline static std::vector<std::pair<ZeroCrossing*, InstanceISRFunc>> instancesISRs{};


        // methods
        static void init(void* instance);
        static void deinit();

        static constexpr int IRAM_ATTR abs(int x) {
            return x < 0 ? -x : x;
        }

        static constexpr unsigned IRAM_ATTR freqToHalfPeriod_us(const unsigned freq) { // NOLINT - intentional recursion
            if(freq == 0)
                return freqToHalfPeriod_us(minFreq);
            return (1000000/freq) / 2;
        }
        // alarm div TIMER_TICK_US = 1/TIMER_TICK_US Hz, TIMER_TICK_US period
        inline static const DRAM_ATTR uint64_t TIMER_TICK_US{freqToHalfPeriod_us(maxFreq)/RESOLUTION_MAX_SIZE-1};

        // HasData
        [[nodiscard]] std::string getWithOptLock(const std::string &key, const bool noLock) const override {
            Logger::logv(TAG, "[getWithOptLock] Getting %s", key.c_str());
            using retType = std::string;
            using FunPtrType = retType(*)(const ZeroCrossing*, const bool noLock);
            return getStringDataHelper<FunPtrType>({
                {FREQUENCY, (FunPtrType)[](const auto, const bool noLock) -> retType { return std::to_string(getFrequency()); }}
            }, key, true, (FunPtrType)[](const auto, const bool) -> retType { return HasData::EMPTY_VALUE; })(this, noLock);
        }

        [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value_raw, const bool noLock, const bool doObjUpdate) override {
            Logger::logv(TAG, "[setWithOptLockAndUpdate] Setting %s->%s", key.c_str(), value_raw.c_str());
            return false; // no variables settable
        }

        static unsigned IRAM_ATTR calcHalfPeriod_us(unsigned xc_elapsed_us);

        static void IRAM_ATTR zeroXPulseISR(void*);

        static void IRAM_ATTR timerISR();
};

#endif // ZERO_CROSSING_H

