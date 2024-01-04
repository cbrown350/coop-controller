#ifndef TESTLOGGERFIXTURE_H_
#define TESTLOGGERFIXTURE_H_

#include <gtest/gtest.h>

#include "TestLogger.h"

class TestLoggerFixture : public ::testing::Test {
public:
    inline static std::shared_ptr<TestLogger> log = std::make_shared<TestLogger>();
    TestLoggerFixture() {
        [[maybe_unused]] static bool singleInit = [this]() {
            Logger<>::addLogger(this->log);
            return true;
        }();
    }
};

#endif // TESTLOGGERFIXTURE_H_