#include "LeadingEdgePhaseDimmer.h"
#include "settings.h"

#include <string>

LeadingEdgePhaseDimmer::LeadingEdgePhaseDimmer(const std::string &instanceID, const uint8_t triacPin,
                                               const bool triacPinPolarityInverted, const uint16_t resolution) :
            ZeroCrossing(instanceID),
            _triacPin(triacPin),
            _triacPinPolarityInverted(triacPinPolarityInverted) {

    setResolution(resolution);
    pinMode(_triacPin, OUTPUT);
    digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);

    isrInit();
}

LeadingEdgePhaseDimmer::~LeadingEdgePhaseDimmer() {
    digitalWrite(_triacPin, LOW ^ _triacPinPolarityInverted);
}

void LeadingEdgePhaseDimmer::setBrightness(const uint16_t brightness, bool noLock) {
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

void LeadingEdgePhaseDimmer::setBrightness(const uint16_t brightness, const uint16_t resolution, const uint16_t minNoFlickerBrightness, bool noLock) {
    Logger::logv(TAG, "[setBrightness 2] Setting brightness to %d", brightness);
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    _brightness = 0; // prevent out of range brightness temporarily
    setResolution(resolution, true);
    setMinNoFlickerBrightness(minNoFlickerBrightness, true);
    setBrightness(brightness, true);
}

void LeadingEdgePhaseDimmer::setPercentApparentBrightness(const uint8_t percent, bool noLock) {
    Logger::logv(TAG, "[setPercentBrightness] Setting percent brightness to %d", percent);
    if(percent > 100) {
        Logger::loge(TAG, "Invalid percent, %d is greater than 100", percent);
        return;
    }
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    const double percHalfPeriod = std::acos(1-(double)2*percent/100)/PI;
    setBrightness((uint16_t)percHalfPeriod *_resolution, true);
}

void LeadingEdgePhaseDimmer::setMinNoFlickerBrightness(const uint16_t minNoFlickerBrightness, bool noLock) {
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

void LeadingEdgePhaseDimmer::setResolution(const uint16_t resolution, bool noLock) {
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

uint16_t LeadingEdgePhaseDimmer::getResolution(bool noLock) const {
    Logger::logv(TAG, "[getResolution] Getting resolution");
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    return _resolution;
}

uint16_t LeadingEdgePhaseDimmer::getBrightness(bool noLock) const {
    Logger::logv(TAG, "[getBrightness] Getting brightness");
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    return _brightness;
}

uint8_t LeadingEdgePhaseDimmer::getPercentApparentBrightness(bool noLock) const {
    Logger::logv(TAG, "[getPercentBrightness] Getting percentbrightness");
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    return (uint8_t)std::round((0.5 - std::cos(((double)getBrightness(true)/_resolution)*PI)/2)*100);
}

uint16_t LeadingEdgePhaseDimmer::getMinNoFlickerBrightness(bool noLock) const {
    Logger::logv(TAG, "[getMinNoFlickerBrightness] Getting minNoFlickerBrightness");
    std::unique_lock l{_dataMutex, std::defer_lock};
    if(!noLock)
        l.lock();
    return _minNoFlickerBrightness;
}


void LeadingEdgePhaseDimmer::timerISRCall(ZeroCrossing* instance, const unsigned currCyclePercentage) {
    auto *thisOne = (LeadingEdgePhaseDimmer*)instance;
    const unsigned onPercentage = RESOLUTION_MAX_SIZE - (RESOLUTION_MAX_SIZE * thisOne->_brightness/thisOne->_resolution);
    if ((currCyclePercentage > onPercentage || thisOne->_brightness == thisOne->_resolution) && thisOne->_brightness != 0) {
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

