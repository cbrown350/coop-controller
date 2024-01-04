#include <gtest/gtest.h>
#include <gmock/gmock.h>

// pio test -vve desktop -f desktop/test_all_libs
int main(int argc, char **argv) {

    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}