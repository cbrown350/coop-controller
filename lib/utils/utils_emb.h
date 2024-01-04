#ifndef CODE_UTILS_EMB_H
#define CODE_UTILS_EMB_H

#include <FreeRTOSConfig.h>
#include <freertos/portmacro.h>

inline unsigned esp_int_level[portNUM_PROCESSORS];
#define timeCriticalEnter() \
    esp_int_level[xPortGetCoreID()] = portSET_INTERRUPT_MASK_FROM_ISR()
#define timeCriticalExit() \
    portCLEAR_INTERRUPT_MASK_FROM_ISR(esp_int_level[xPortGetCoreID()])

#endif //CODE_UTILS_EMB_H
