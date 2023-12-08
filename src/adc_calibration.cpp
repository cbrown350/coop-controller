/*
* Adapted from code discussed in https://github.com/espressif/arduino-esp32/issues/1804
*/

#include "adc_calibration.h"

namespace adc {
  const esp_adc_cal_characteristics_t *adc_chars = new esp_adc_cal_characteristics_t;

  // ========= setup_adc =========
  void setup_adc(adc_unit_t adc_num,
                      adc_atten_t atten,
                      adc_bits_width_t bit_width,
                      uint32_t default_vref) {
    // esp_adc_cal_value_t val_type
      esp_adc_cal_characterize(adc_num, atten, bit_width, default_vref, const_cast<esp_adc_cal_characteristics_t*>(adc_chars));
  }


  // ========= analogRead_cal =========
  int analogRead_cal_mv(uint8_t channel, adc_atten_t attenuation) {
    adc1_channel_t channelNum = ADC1_CHANNEL_0;

    /*
      Set number of cycles per sample
      Default is 8 and seems to do well
      Range is 1 - 255
    * */
    // \analogSetCycles(uint8_t cycles);

    /*
      Set number of samples in the range.
      Default is 1
      Range is 1 - 255
      This setting splits the range into
      "samples" pieces, which could look
      like the sensitivity has been multiplied
      that many times
    * */
    // \analogSetSamples(uint8_t samples);

    switch (channel) {
      case (GPIO_NUM_36):
        channelNum = ADC1_CHANNEL_0;
        break;

      case (GPIO_NUM_39):
        channelNum = ADC1_CHANNEL_3;
        break;

      case (GPIO_NUM_34):
        channelNum = ADC1_CHANNEL_6;
        break;

      case (GPIO_NUM_35):
        channelNum = ADC1_CHANNEL_7;
        break;

      case (GPIO_NUM_32):
        channelNum = ADC1_CHANNEL_4;
        break;

      case (GPIO_NUM_33):
        channelNum = ADC1_CHANNEL_5;
        break;
    }

    adc1_config_channel_atten(channelNum, attenuation);
    return esp_adc_cal_raw_to_voltage(analogRead(channel), adc_chars);
  }

} // namespace adc  