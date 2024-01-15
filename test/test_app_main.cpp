#include <gtest/gtest.h>
#include <gmock/gmock.h>

#if defined(ARDUINO)
#include "embedded/system_setup_test.h"

// pio test -vve esp32-debug -f embedded/debug/test_all_libs
// pio test -vve esp32-release -f embedded/release/test_all_libs
extern "C" void app_main() {
    // must put Arduino init, Serial setup, and wifi setup before InitGoogleTest
    system_setup_test::setupWithWifi();
    ::testing::InitGoogleTest();
    ::testing::InitGoogleMock();
    exit(RUN_ALL_TESTS());
}

#else
int main(int argc, char **argv) {

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

#endif