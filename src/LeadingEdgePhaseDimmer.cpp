//
// Created by cbrow on 2023-12-19.
//

#include "LeadingEdgePhaseDimmer.h"
#include "coop_settings.h"
#include "CoopLogger.h"
#include <esp_timer.h>
#include "utils.h"
#include <vector>

LeadingEdgePhaseDimmer::LeadingEdgePhaseDimmer(const std::string &instanceID, const uint8_t triacPin,
                                               const bool triacPinPolarityInverted, const uint16_t resolution) :
            HasData(instanceID, LeadingEdgePhaseDimmer::readOnlyKeys),
            _triacPin(triacPin),
            _triacPinPolarityInverted(triacPinPolarityInverted) {

        setResolution(resolution);
        pinMode(_triacPin, OUTPUT);
        digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);
        std::scoped_lock l(instancesMutex);
        if(instances.empty())
            init();
        instances.push_back(this);
}

void LeadingEdgePhaseDimmer::init() {
    pinMode(ZERO_CROSS_IN_B, INPUT_PULLUP);
//    attachInterrupt(ZERO_CROSS_IN_B, &LeadingEdgePhaseDimmer::zeroXPulseISR, CHANGE);
    attachInterrupt(ZERO_CROSS_IN_B, &LeadingEdgePhaseDimmer::zeroXPulseISR, FALLING);
    CoopLogger::logv(TAG, "[init] ZERO_CROSS_IN_B: %d", ZERO_CROSS_IN_B);

//    gpio_pad_select_gpio(ZERO_CROSS_IN_B);
//    gpio_set_direction(ZERO_CROSS_IN_B, GPIO_MODE_INPUT);
//    gpio_pullup_en(ZERO_CROSS_IN_B);
//    gpio_pulldown_dis(ZERO_CROSS_IN_B);
//
//    gpio_set_intr_type(ZERO_CROSS_IN_B, GPIO_INTR_ANYEDGE);
////    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL4 | ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED);
////    gpio_isr_handler_add(ZERO_CROSS_IN_B, LeadingEdgePhaseDimmer::zeroXPulseISR, (void*) ZERO_CROSS_IN_B);
////    gpio_isr_register(LeadingEdgePhaseDimmer::zeroXPulseISR, (void *) "Test", 0, NULL);
//    gpio_intr_enable(ZERO_CROSS_IN_B);
//    esp_intr_alloc( ETS_GPIO_INTR_SOURCE, ESP_INTR_FLAG_LEVEL3 | ESP_INTR_FLAG_IRAM , LeadingEdgePhaseDimmer::zeroXPulseISR, (void *) "Test", NULL);
//    // component config -> esp system settings; component config-> freertos - tick source timer; comp config -> high res timer

    Timer0_Cfg = timerBegin(0, 80, true); // 80MHz / 80 divider = 1MHz
    timerAttachInterrupt(Timer0_Cfg, &LeadingEdgePhaseDimmer::timerISR, true);
    timerAlarmWrite(Timer0_Cfg, TIMER_TICK_US, true);
    timerAlarmEnable(Timer0_Cfg);
    CoopLogger::logv(TAG, "[init] Timer0_Cfg: %p", Timer0_Cfg);
}

void LeadingEdgePhaseDimmer::deinit() {
    timerAlarmDisable(Timer0_Cfg);
    timerDetachInterrupt(Timer0_Cfg);
    timerEnd(Timer0_Cfg);
    detachInterrupt(ZERO_CROSS_IN_B);
}

LeadingEdgePhaseDimmer::~LeadingEdgePhaseDimmer() {
    digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);
    std::scoped_lock l(instancesMutex);
    instances.erase(std::remove(instances.begin(), instances.end(), this), instances.end());
    if(instances.empty())
        deinit();
}


constexpr int avgSize = 5;
unsigned long halfPeriod_us_RunningAvg[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
void IRAM_ATTR LeadingEdgePhaseDimmer::zeroXPulseISR() {
    detachInterrupt(ZERO_CROSS_IN_B); // disable further interrupts due to glitches
    timeCriticalEnter();

//    static unsigned long lastZeroXFallingEdgeTime_us = esp_timer_get_time();

//    const unsigned long now_us = esp_timer_get_time();
    const unsigned now_us = xthal_get_ccount() / 240;

//    const bool isFallingEdge = !digitalRead(ZERO_CROSS_IN_B);
    const bool isFallingEdge = (GPIO.in >> ZERO_CROSS_IN_B) ^ 1;

    const auto sinceLastFalling_us = now_us - lastZeroXFallingEdgeTime_us;
    if (isFallingEdge) {
        lastZeroXFallingEdgeTime_us = now_us;
        if(8000 < sinceLastFalling_us && sinceLastFalling_us < 10100) {
            unsigned long newHalfPeriod_us = 0;
            for (int i = 0; i < avgSize - 1; i++) {
                halfPeriod_us_RunningAvg[i] = halfPeriod_us_RunningAvg[i + 1];
                newHalfPeriod_us += halfPeriod_us_RunningAvg[i];
            }
            halfPeriod_us_RunningAvg[avgSize - 1] = sinceLastFalling_us;
            newHalfPeriod_us += sinceLastFalling_us;
            //        halfPeriod_us = sinceLastFalling_us;
            halfPeriod_us = 8333;//newHalfPeriod_us / avgSize;
            //        periodUs = halfPeriod_us * 2;
//            lastZeroXFallingEdgeTime_us = now_us;
            //        zeroXCond.notify_all();
            //        estimatedZeroXSysTimeUs = now_us + zeroXFallingToZeroOffset_us;
            // adjust timerTicks to proper count based on width of zero crossing pulse and hysteresis
            timerTicks = (int)(-(zeroXHalfPulseWidth_us - HYSTERESIS_OFFSET_US) / TIMER_TICK_US);//(unsigned) ((sinceLastFalling_us - zeroXHalfPulseWidth_us) / TIMER_TICK_US);

            //        for (auto &instance: instances)
            //            instance->toggleTriacPin(0);
        } // else ignore, probably noise
    } else {
        if(sinceLastFalling_us < 500) {
            const unsigned zeroXHalfPulseWidth_us_local = sinceLastFalling_us / 2;
            zeroXHalfPulseWidth_us = 140;//zeroXHalfPulseWidth_us_local;
//            timerTicks = (int) ((sinceLastFalling_us - zeroXHalfPulseWidth_us_local) / TIMER_TICK_US);
            timerTicks = (int) ((zeroXHalfPulseWidth_us_local - HYSTERESIS_OFFSET_US) / TIMER_TICK_US);

            //        for (auto &instance: instances) {
            //            auto delay = 8000-8000*instance->_brightness/instance->_resolution;
            //            while(xthal_get_ccount()/240 - now_us < delay) {}
            //            instance->toggleTriacPin(RESOLUTION_MAX_SIZE);
            //        }
        } // else ignore, probably noise
    }
//    digitalWrite(RED_LED_OUT_B, isFallingEdge);
    if(isFallingEdge)
        GPIO.out_w1tc = 1<<RED_LED_OUT_B;
    else
        GPIO.out_w1ts = 1<<RED_LED_OUT_B;

    timeCriticalExit();
}

static unsigned long lastTimerTickTimeUs{0};
void IRAM_ATTR LeadingEdgePhaseDimmer::timerISR() {
//    static unsigned long lastTimerTickTimeUs = esp_timer_get_time();

    timeCriticalEnter();
//    unsigned long now_us = esp_timer_get_time();
    const unsigned now_us = xthal_get_ccount() / 240;
//    unsigned long elapsedUs = now_us - lastTimerTickTimeUs;
//    lastTimerTickTimeUs = now_us;


    int currTimerTicks = timerTicks/1;

    if(currTimerTicks+TIMER_TICK_US > 1000) // re-enable zero crossing interrupt long after outside glitch window
        attachInterrupt(ZERO_CROSS_IN_B, &LeadingEdgePhaseDimmer::zeroXPulseISR, FALLING);

    const unsigned halfPeriod_us_local = halfPeriod_us;
    const unsigned maxTicks = halfPeriod_us_local / TIMER_TICK_US;
//    const unsigned maxTicks = 8333 / TIMER_TICK_US;
//    currTimerTicks += (int)round((now_us - lastTimerTickTimeUs) / TIMER_TICK_US); // adjust for missed/delayed ISRs
    currTimerTicks = (int)((lastZeroXFallingEdgeTime_us + zeroXHalfPulseWidth_us) / TIMER_TICK_US);
    if(currTimerTicks > maxTicks)
        currTimerTicks = 0;

    const unsigned currTicksCalc = currTimerTicks >= 0 ? currTimerTicks : (unsigned)((halfPeriod_us_local + currTimerTicks * TIMER_TICK_US) / TIMER_TICK_US);
    const unsigned currPercentage = maxTicks > 0 ? currTicksCalc * (unsigned)(RESOLUTION_MAX_SIZE / maxTicks) : 0;

    for (auto &instance: instances)
        instance->toggleTriacPin(currPercentage);

    timerTicks = currTimerTicks;

//    timerTickCond.notify_all();

    lastTimerTickTimeUs = now_us;

    timeCriticalExit();
}

void IRAM_ATTR LeadingEdgePhaseDimmer::toggleTriacPin(const unsigned currPercentage) {
//    digitalWrite(GREEN_LED_OUT_B, !digitalRead(GREEN_LED_OUT_B));
    if((GPIO.out >> GREEN_LED_OUT_B) & 1)
        GPIO.out_w1tc = 1<<GREEN_LED_OUT_B;
    else
        GPIO.out_w1ts = 1<<GREEN_LED_OUT_B;

    if(_brightness > 0 && currPercentage > 0) {
        const auto onPercentage = RESOLUTION_MAX_SIZE - (_brightness * RESOLUTION_MAX_SIZE / _resolution);
        const bool isZeroX = false;//(GPIO.in >> ZERO_CROSS_IN_B) ^ 1;
        if (currPercentage >= onPercentage && !isZeroX) {
            // turn on triac
//            digitalWrite(_triacPin, HIGH ^ _triacPinPolarityInverted);
            if(_triacPinPolarityInverted)
                GPIO.out_w1tc = 1<<_triacPin;
            else
                GPIO.out_w1ts = 1<<_triacPin;
            return;
        }
    }

    GPIO.out_w1ts = 1<<RED_LED_OUT_B;
    // turn off triac
//    digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);
    if(_triacPinPolarityInverted)
        GPIO.out_w1ts = 1<<_triacPin;
    else
        GPIO.out_w1tc = 1<<_triacPin;
}

