#ifndef FACTORY_RESET_H_
#define FACTORY_RESET_H_

#include <Logger.h>
#include <functional>

namespace factory_reset {
    using Logger = Logger<>;

    void init();
    void addOnFactoryResetCallback(const std::function<void()> &onFactoryResetCallback);
}


#endif // FACTORY_RESET_H_