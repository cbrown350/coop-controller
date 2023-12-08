#ifndef FACTORY_RESET_H_
#define FACTORY_RESET_H_

#include <functional>

namespace factory_reset {
    
    void init();
    void addOnFactoryResetCallback(const std::function<void()> onFactoryResetCallback);
}


#endif // FACTORY_RESET_H_