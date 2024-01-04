#ifndef DESKTOPLOGGERFIXTURE_H_
#define DESKTOPLOGGERFIXTURE_H_

#include <gtest/gtest.h>

#include "DesktopLogger.h"

class DesktopLoggerFixture : public ::testing::Test {
public:
    inline static std::shared_ptr<DesktopLogger> log = std::make_shared<DesktopLogger>();
    DesktopLoggerFixture() {
        [[maybe_unused]] static bool singleInit = [this]() {
            Logger<>::addLogger(this->log);
            return true;
        }();
    }
};

#endif // DESKTOPLOGGERFIXTURE_H_