#include "ZeroCrossing.h"
#include "settings.h"

#include <esp_intr_alloc.h>

#include <Logger.h>

#include <string>
#include <algorithm>

ZeroCrossing::ZeroCrossing(const std::string &instanceID) :
            HasData(instanceID) {
        std::scoped_lock l(instancesMutex);
        if(instancesISRs.empty()) {
            xTaskCreatePinnedToCore(
                    init,       //Function to implement the task
                    "ZroXg_Init", //Name of the task (16 chars max)
                    3192,       //Stack size in words
                    this,       //Task input parameter
                    0,          //Priority of the task
                    nullptr,       //Task handle.
                    1);
        }
}

void ZeroCrossing::init(void* instance) {
#ifdef XC_GPIO_DEBUG_OUT      
      pinMode(XC_GPIO_DEBUG_OUT, OUTPUT);
#endif
#ifdef TIMER_GPIO_DEBUG_OUT      
      pinMode(TIMER_GPIO_DEBUG_OUT, OUTPUT);
#endif
    std::scoped_lock l(instancesMutex);
    ((ZeroCrossing*)instance)->addInstanceTimerISR();

    // using lower level drivers to be able to set flags for ISRs

    gpio_pad_select_gpio(ZERO_CROSS_IN_B);
    gpio_set_direction(ZERO_CROSS_IN_B, GPIO_MODE_INPUT);
    gpio_pullup_en(ZERO_CROSS_IN_B);
    gpio_pulldown_dis(ZERO_CROSS_IN_B);
    gpio_set_intr_type(ZERO_CROSS_IN_B, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3);
    gpio_isr_handler_add(ZERO_CROSS_IN_B, ZeroCrossing::zeroXPulseISR,nullptr);
    gpio_intr_enable(ZERO_CROSS_IN_B);
    Logger::logv(TAG, "[init] ZERO_CROSS_IN_B: %d", ZERO_CROSS_IN_B);

    Timer0_Cfg = timerBegin(0, 80, true); // 80MHz / 80 divider = 1MHz
    timerAttachInterruptFlag(Timer0_Cfg, &ZeroCrossing::timerISR, true, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3);
    timerAlarmWrite(Timer0_Cfg, TIMER_TICK_US, true);
    timerAlarmEnable(Timer0_Cfg);
    Logger::logv(TAG, "[init] Timer0_Cfg: %p", Timer0_Cfg);

    vTaskDelete(nullptr);
}

void ZeroCrossing::deinit() {
    gpio_intr_disable(ZERO_CROSS_IN_B);
    gpio_isr_handler_remove(ZERO_CROSS_IN_B);

    timerAlarmDisable(Timer0_Cfg);
    timerDetachInterrupt(Timer0_Cfg);
    timerEnd(Timer0_Cfg);
}

ZeroCrossing::~ZeroCrossing() {
    std::scoped_lock l(instancesMutex);
    instancesISRs.erase(std::remove_if(instancesISRs.begin(), instancesISRs.end(), [this](const auto &instanceISR) {
        return instanceISR.first == this;
    }), instancesISRs.end());
    if(instancesISRs.empty())
        deinit();
}

unsigned ZeroCrossing::calcHalfPeriod_us(const unsigned xc_elapsed_us) {
    const unsigned actualFreq = halfPeriod_us_toFreq(xc_elapsed_us);
    unsigned closestFreq = maxFreq;
    for(const unsigned freq: VALID_FREQUENCIES_HZ) {
        if(abs((int)freq - (int)actualFreq) < abs((int)closestFreq - (int)actualFreq))
            closestFreq = freq;
    }
    const unsigned calcHalfPeriodAvg = freqToHalfPeriod_us(closestFreq);
    const double calcFreqAvg = closestFreq; //halfPeriod_us_toFreq(calcHalfPeriodAvg);
    return calcFreqAvg < (double)minFreq * (1-freqMargin) ? freqToHalfPeriod_us(minFreq)
                                                          : (calcFreqAvg > (double)maxFreq * (1+freqMargin)
                                                             ? freqToHalfPeriod_us(maxFreq)
                                                             : calcHalfPeriodAvg);
}

void ZeroCrossing::zeroXPulseISR(void* ) {
    timeCriticalEnter();
    const unsigned long now_us = esp_timer_get_time(); //xthal_get_ccount() / 240;
    const unsigned elapsed_us = now_us - lastXCTime;
    static constexpr DRAM_ATTR unsigned minHalfPeriod_us = freqToHalfPeriod_us(maxFreq * (1+freqMargin));
    if(!maskXC && elapsed_us >= minHalfPeriod_us) { // ignore glitches
        maskXC = true; // mask to ensure only called once, glitch filter
        lastXCTime = now_us;
        xCTime = now_us + XC_HALF_PULSE_WIDTH_US - HYSTERESIS_OFFSET_US;
        halfPeriod_us = calcHalfPeriod_us(elapsed_us);
#ifdef XC_GPIO_DEBUG_OUT
        GPIO.out_w1tc = 1<<XC_GPIO_DEBUG_OUT;
#endif
    }
    timeCriticalExit();
}

void ZeroCrossing::timerISR() {
    timeCriticalEnter();
    const unsigned now_us = esp_timer_get_time(); //xthal_get_ccount() / 240;
    const unsigned halfPeriod_us_local = halfPeriod_us;
    const int elapsed_us_actual = ((int)now_us - (int)xCTime) % (int)halfPeriod_us_local; // mod checks for missed zero crossings
    if(elapsed_us_actual > 0) { // actual xCTime may be in the future (elapsed_us_actual negative) if XC was just triggered
        const auto elapsed_us = (unsigned)elapsed_us_actual;

        if(elapsed_us > halfPeriod_us_local/2 && maskXC) { // re-enable zero crossing interrupt long after outside glitch window
            maskXC = false;
#ifdef XC_GPIO_DEBUG_OUT
            GPIO.out_w1ts = 1<<XC_GPIO_DEBUG_OUT;
#endif
        }

        const unsigned currCyclePercentage = RESOLUTION_MAX_SIZE * elapsed_us/(halfPeriod_us_local); // - XC_HALF_PULSE_WIDTH_US
        const unsigned currCyclePercentageClamped = currCyclePercentage > RESOLUTION_MAX_SIZE ? RESOLUTION_MAX_SIZE : currCyclePercentage;
        for (auto &instanceISR: instancesISRs) // call instances using pair {this, timerISR}
            instanceISR.second(instanceISR.first, currCyclePercentageClamped);
    }
    timeCriticalExit();

#ifdef TIMER_GPIO_DEBUG_OUT
    if((GPIO.out >> TIMER_GPIO_DEBUG_OUT) & 1)
            GPIO.out_w1tc = 1<<TIMER_GPIO_DEBUG_OUT;
        else
            GPIO.out_w1ts = 1<<TIMER_GPIO_DEBUG_OUT;
#endif

}

