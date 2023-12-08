/*
 * Copyright 2023 Clifford B. Brown
 *   Major portions of this code are from James Sleeman, http://sparks.gogo.co.nz/ntc.html
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <math.h>
#include "adc_temp_conversion.h"


namespace adc {
  float convertAnalogToTemperature(unsigned int analogReadValue);
  float approximateTemperatureFloat(unsigned int analogReadValue);
  int approximateTemperatureInt(unsigned int analogReadValue);

  float convertTempCtoF(float tempC) {
    return (tempC * 9.0f / 5.0f) + 32.0f;
  }

  float convertAnalogToTemperatureF(unsigned int analogReadValue) {
    return convertTempCtoF(convertAnalogToTemperature(analogReadValue));
  }

  float approximateTemperatureFloatF(unsigned int analogReadValue) {
    return convertTempCtoF(approximateTemperatureFloat(analogReadValue));
  }

  int approximateTemperatureIntF(unsigned int analogReadValue) {
    return static_cast<int>(convertTempCtoF((float)approximateTemperatureInt(analogReadValue)));
  }

  float convertAnalogToTemperatureC(unsigned int analogReadValue) {
    return convertAnalogToTemperature(analogReadValue);
  }

  float approximateTemperatureFloatC(unsigned int analogReadValue) {
    return approximateTemperatureFloat(analogReadValue);
  }

  int approximateTemperatureIntC(unsigned int analogReadValue) {
    return approximateTemperatureInt(analogReadValue);
  }


  // Simple interpolation method

  // int NTC_table[33] = {
  //   1233, 994, 755, 625, 535, 467, 411, 363, 
  //   321, 284, 250, 218, 189, 161, 134, 108, 83, 
  //   58, 33, 8, -16, -41, -67, -94, -122, -151, 
  //   -184, -219, -260, -309, -373, -471, -569
  // };
  
  // **
  // * \brief    Converts the ADC result into a temperature value.
  // *
  // *           P1 and p2 are the interpolating point just before and after the
  // *           ADC value. The function interpolates between these two points
  // *           The resulting code is very small and fast.
  // *           Only one integer multiplication is used.
  // *           The compiler can replace the division by a shift operation.
  // *
  // *           In the temperature range from -25°C to 55°C the error
  // *           caused by the usage of a table is 0.273°C
  // *
  // * \param    adc_value  The converted ADC result
  // * \return              The temperature in 0.1 °C
  // *
  // */
  // int NTC_ADC2Temperature(unsigned int adc_value) { 
  //   int p1,p2;
  //  * Estimate the interpolating point before and after the ADC value. */
  //   p1 = NTC_table[ (adc_value >> 5)  ];
  //   p2 = NTC_table[ (adc_value >> 5)+1];
  
  //   * Interpolate between both points. */
  //   \return p1 - ( (p1-p2) * (adc_value & 0x001F) ) / 32;
  // };


  // More complex calculation method

  // <== AFTER PASTING IN YOUR CODE THERE MIGHT BE A QUOTE MARK HERE, DELETE IT
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // ADC Value to Temperature for NTC Thermistor.
  // Author: James Sleeman http://sparks.gogo.co.nz/ntc.html
  // Licence: BSD (see footer for legalese)
  //
  // Thermistor characteristics:
  //   Nominal Resistance 10000 at 25°C
  //   Beta Value 3950
  //
  // Usage Examples:
  //   float bestAccuracyTemperature    = convertAnalogToTemperature(analogRead(analogPin));
  //   float lesserAccuracyTemperature  = approximateTemperatureFloat(analogRead(analogPin));
  //   int   lowestAccuracyTemperature  = approximateTemperatureInt(analogRead(analogPin));
  //
  // Better accuracy = more resource (memory, flash) demands, the approximation methods 
  // will only produce reasonable results in the range -25-55°C
  //
  //
  // Thermistor Wiring:
  //   Vcc -> [22000 Ohm Resistor] -> Thermistor -> Gnd
  //                             |
  //                             \-> analogPin
  //
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /** Calculate the temperature in °C from ADC (analogRead) value (best accuracy).
   *
   *  This conversion should generate reasonably accurate results over the entire range of 
   *  the thermistor, it implements the common 'Beta' approximation for a thermistor
   *  having Beta of 3950, and nominal values of 10000Ω at 25°C
   *
   *  @param   The result of an ADC conversion (analogRead) in the range 0 to 1023
   *  @return  Temperature in °C
   */

  float  convertAnalogToTemperature(unsigned int analogReadValue)
  {
    // If analogReadValue is 1023, we would otherwise cause a Divide-By-Zero,
    // Treat as crazy out-of-range temperature.
    if(analogReadValue == 1023) return 1000.0; 
                                                
    return static_cast<float>((1/((log(((22000.0 * analogReadValue) / (1023.0 - analogReadValue))/10000.0)/3950.0) + (1 / (273.15 + 25.000)))) - 273.15);
  }




  /** Approximate the temperature in °C from ADC (analogRead) value, using floating-point math.
   *
   *  This approximation uses floating point math, but much less complex so may be useful for 
   *  improved performance, reducing program memory consumption, and reducing runtime memory
   *  usage.
   *
   *  This conversion has the following caveats...
   *    Suitable Range              : -25°C to 55°C 
   *    Average Error (Within Range): +/- 2.255 °C°C
   *    Maximum Error (Within Range): 7.835 °C°C
   *
   *  This approximation implements a linear regression of the Beta approximation 
   *  for a thermistor having Beta of 3950, and nominal values of 10000Ω at 
   *  25°C calculated for temperatures across the range above.
   *
   * @param   The result of an ADC conversion (analogRead) in the range 0 to 1023
   * @return  Temperature in °C (+/- 7.835 °C)
   */

  float  approximateTemperatureFloat(unsigned int analogReadValue)
  {
    return static_cast<float>(-0.0948484354554803*analogReadValue+58.7363978660206);
  }

  /** Approximate the temperature in °C from ADC (analogRead) value, using integer math.
   *
   *  This approximation uses only integer math, so has a subsequent resolution of
   *  of only 1°C, but for very small microcontrollers this is useful as floating point
   *  math eats your program memory.
   *
   *  This conversion has the following caveats...
   *    Suitable Range              : -25°C to 55°C 
   *    Average Error (Within Range): +/- 2.232 °C°C
   *    Maximum Error (Within Range): 8.000 °C°C
   *
   *  This approximation implements a linear regression of the Beta approximation 
   *  for a thermistor having Beta of 3950, and nominal values of 10000Ω at 
   *  25°C calculated for temperatures across the range above.
   *
   * @param   The result of an ADC conversion (analogRead) in the range 0 to 1023
   * @return  Temperature in °C (+/- 8.000 °C)
   */

  int approximateTemperatureInt(unsigned int analogReadValue)
  {
    return ((((long)analogReadValue*15) / -158) + 59) - 1;
  }


  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Legal Mumbo Jumo Follows, in short: do whatever you want, just don't sue me.
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Copyright © 2023 James Sleeman. All Rights Reserved.
  // 
  // Redistribution and use in source and binary forms, with or without 
  // modification, are permitted provided that the following conditions are met:
  // 
  // 1. Redistributions of source code must retain the above copyright notice,
  //  this list of conditions and the following disclaimer.
  // 
  // 2. Redistributions in binary form must reproduce the above copyright notice,
  //  this list of conditions and the following disclaimer in the documentation 
  //  and/or other materials provided with the distribution.
  // 
  // 3. The name of the author may not be used to endorse or promote products 
  //  derived from this software without specific prior written permission.
  // 
  // THIS SOFTWARE IS PROVIDED BY James Sleeman AS IS AND ANY EXPRESS OR IMPLIED
  //  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  //  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
  //  EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  //  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
  //  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
  //  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  //  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  //  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  //  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  //
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"			
} // namespace								