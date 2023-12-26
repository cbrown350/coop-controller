/*
* Adapted from code discussed in https://github.com/espressif/arduino-esp32/issues/1804
*/
#ifndef ADC_CALIBRATION_H
#define ADC_CALIBRATION_H

#include <Arduino.h>
#include <esp_adc_cal.h>

namespace adc {
    void setup_adc(adc_unit_t adc_num,
                        adc_atten_t atten,
                        adc_bits_width_t bit_width,
                        uint32_t default_vref);
    unsigned analogRead_cal_mv(uint8_t channel, adc_atten_t attenuation);

} // namespace adc

#endif // ADC_CALIBRATION_H