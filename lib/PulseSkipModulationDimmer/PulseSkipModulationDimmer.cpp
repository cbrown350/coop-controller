#include <PulseSkipModulationDimmer.h>


PulseSkipModulationDimmer::PulseSkipModulationDimmer(const std::string &instanceID, const uint8_t triacPin,
                                               const bool triacPinPolarityInverted, const uint16_t cycles) :
        ZeroCrossing(instanceID),
        _triacPin(triacPin),
        _triacPinPolarityInverted(triacPinPolarityInverted) {

    setCycles(cycles);
    pinMode(_triacPin, OUTPUT);
    digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);
}

PulseSkipModulationDimmer::~PulseSkipModulationDimmer() {
    digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);
}

void PulseSkipModulationDimmer::timerISRCall(ZeroCrossing* instance, const unsigned currCyclePercentage) {
    auto *thisOne = (PulseSkipModulationDimmer*)instance;
    if(currCyclePercentage > RESOLUTION_MAX_SIZE/2) { // only need to calculate once at beginning of half cycle
        thisOne->currHalfCycleCalcDone = false; // only enable after half of period in case missed ISRs
        return;
    }

    if(thisOne->currHalfCycleCalcDone) // only calc once per half cycle
        return;
    thisOne->halfCycleCount++;
    thisOne->currHalfCycleCalcDone = true;
    if(thisOne->halfCycleCount%2 != 0) // ensure both halves of AC cycle, only change on even half cycles
        return;

    if(thisOne->halfCycleCount/2 > thisOne->_cycles) {
        thisOne->halfCycleCount = 0;
        thisOne->cyclesFiredCount = 0;
    }

    const bool shouldFire = thisOne->_brightness > // check for div by 0
            (thisOne->halfCycleCount == 0 ? 0 : (thisOne->_cycles * thisOne->cyclesFiredCount)/(thisOne->halfCycleCount/2) - 1);

    if (shouldFire) {
        thisOne->cyclesFiredCount++;
        // turn on triac
        if(thisOne->_triacPinPolarityInverted)
            GPIO.out_w1tc = 1<<thisOne->_triacPin;
        else
            GPIO.out_w1ts = 1<<thisOne->_triacPin;
        return;
    }

    // turn off triac
    if(thisOne->_triacPinPolarityInverted)
        GPIO.out_w1ts = 1<<thisOne->_triacPin;
    else
        GPIO.out_w1tc = 1<<thisOne->_triacPin;
}

void PulseSkipModulationDimmer::setBrightness(const uint16_t brightness, bool noLock) {
    Logger::logv(TAG, "[setBrightness 1] Setting brightness to %d", brightness);
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    uint16_t tmpCycles = _cycles;
    if(brightness > tmpCycles) {
        Logger::loge(TAG, "brightness %d is greater than allowable (cycles) %d", brightness, tmpCycles);
        return;
    }
    this->_brightness = brightness;
}

void PulseSkipModulationDimmer::setBrightness(const uint16_t brightness, const uint16_t cycles, bool noLock) {
    Logger::logv(TAG, "[setBrightness 2] Setting brightness to %d", brightness);
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    _brightness = 0; // prevent out of range brightness temporarily
    setCycles(cycles, true);
    setBrightness(brightness, true);
}

void PulseSkipModulationDimmer::setCycles(const uint16_t cycles, bool noLock) {
    Logger::logv(TAG, "[setResolution] Setting resolution to %d", cycles);
    if(cycles > max_cycles || cycles < 2) {
        Logger::loge(TAG, "cycles %d is out of range (min 2 to max %d)", cycles, max_cycles);
        return;
    }
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    _brightness = 0; // prevent out of range brightness temporarily
    const auto newBrightness = _brightness * cycles / _cycles;
    Logger::logv(TAG, "Setting cycles to %d (and brightness to %d)", cycles, newBrightness);
    _cycles = cycles;
    setBrightness(newBrightness, true);
}

uint16_t PulseSkipModulationDimmer::getCycles(bool noLock) const {
    Logger::logv(TAG, "[getResolution] Getting resolution");
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    (void)noLock;
    return _cycles;
}

uint16_t PulseSkipModulationDimmer::getBrightness(bool noLock) const {
    Logger::logv(TAG, "[getBrightness] Getting brightness");
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    return _brightness;
}
