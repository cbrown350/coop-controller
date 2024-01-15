#ifndef OTA_UPDATE_H_
#define OTA_UPDATE_H_

#include <Logger.h>

namespace ota_update {
    using Logger = Logger<>;

    inline static constexpr const char * const TAG{"otaud"};

    void startLoop(bool restartOnUpdate = false);
    void stopLoop();
} // namespace ota_update

#endif // OTA_UPDATE_H_