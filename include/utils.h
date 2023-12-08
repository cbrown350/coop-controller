#ifndef UTILS_H_
#define UTILS_H_

#include <mutex>
#include <condition_variable>

namespace utils {    
    template<class Duration>
    bool wait_for(Duration duration, std::mutex &m, std::condition_variable &c, const bool &stop) {
        std::unique_lock<std::mutex> l(m);
        return !c.wait_for(l, duration, [&stop]() { return stop; });
    };
    
} // namespace utils

#endif // UTILS_H_