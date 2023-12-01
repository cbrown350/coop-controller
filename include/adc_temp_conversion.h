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

#ifndef ADC_TEMP_CONVERSION_H
#define ADC_TEMP_CONVERSION_H

// Functions to convert the analogRead() value to a temperature in degrees C or F.
extern float convertAnalogToTemperatureF(unsigned int analogReadValue);
extern float approximateTemperatureFloatF(unsigned int analogReadValue);
extern int approximateTemperatureIntF(unsigned int analogReadValue);
extern float convertAnalogToTemperatureC(unsigned int analogReadValue);
extern float approximateTemperatureFloatC(unsigned int analogReadValue);
extern int approximateTemperatureIntC(unsigned int analogReadValue);

#endif // ADC_TEMP_CONVERSION_H