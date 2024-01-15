#include <gtest/gtest.h>

#include "../../TestLoggerFixture.h"
#include "../TestHasDataObj.h"

#include "settings.h"
#include <ntp_time.h>

inline static constexpr const char * TAG{"tntp"};

class TestNTP : public TestLoggerFixture { };

TEST_F(TestNTP, TestMQTTConnect) {    
//     TestHasDataObj test_data2{"test_data2"};
//   [[maybe_unused]] const auto no_use = ntp_time::getData().set(ntp_time::TIMEZONE, "America/New_York");
}