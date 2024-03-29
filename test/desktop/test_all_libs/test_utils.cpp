#include <gtest/gtest.h>

#include <thread>
#include <condition_variable>
#include <string>
#include <vector>
#include <mutex>

#include "../DesktopLogger.h"

#include <utils.h>

inline static constexpr const char * TAG{"tutils"};

TEST(UtilsTests, UtilsStringFormat) {
    DesktopLogger::logd(TAG, "utils_string_format");

    std::string str = utils::string_format("test %d", 1);

    EXPECT_EQ("test 1", str);
}

TEST(UtilsTests, UtilsWaitForFlag) {
    DesktopLogger::logd(TAG, "utils_wait_for_flag");

    constexpr unsigned timeout = 4;

    bool stopFlag{false};
    std::mutex m;
    std::condition_variable c;
    std::thread t([&stopFlag, &m, &c]() {
        std::this_thread::sleep_for(std::chrono::seconds(timeout/4));
        DesktopLogger::logd(TAG, "thread time passed, notifying...");
        {
            std::scoped_lock l(m);
            stopFlag = true;
        }
        c.notify_all();
        DesktopLogger::logd(TAG, "thread notified");
    });
    unsigned long start = DesktopLogger::millis();
    bool result = utils::wait_for<>(std::chrono::seconds(timeout), m, c, stopFlag);
    unsigned long end = DesktopLogger::millis();
    DesktopLogger::logd(TAG, "wait_for result %d, time %lu", result, end - start);

    EXPECT_TRUE(stopFlag) << "Incorrect flag value after wait_for"; // stopFlag should now be true
    EXPECT_FALSE(result) << "Incorrect result from wait_for"; // stopFlag should now be true, returned and inverted
    EXPECT_TRUE(end - start < timeout * 1000/2) << "Didn't respond to the flag notification";

    pthread_cancel(t.native_handle());
    t.join();
}

TEST(UtilsTests, UtilsWaitForTimeout) {
    DesktopLogger::logd(TAG, "utils_wait_for_timeout");

    constexpr unsigned timeout = 1;

    bool stopFlag{false};
    std::mutex m;
    std::condition_variable c;
    std::thread t([&stopFlag, &m, &c]() {
        std::this_thread::sleep_for(std::chrono::seconds(timeout * 4));
        DesktopLogger::logd(TAG, "thread time passed, notifying...");
        {
            std::scoped_lock l(m);
            stopFlag = true;
        }
        c.notify_all();
        DesktopLogger::logd(TAG, "thread notified");
    });
    const unsigned long start = DesktopLogger::millis();
    const bool result = utils::wait_for<>(std::chrono::seconds(timeout), m, c, stopFlag);
    const unsigned long end = DesktopLogger::millis();
    DesktopLogger::logd(TAG, "wait_for result %d, time %lu", result, end - start);

    EXPECT_FALSE(stopFlag) << "Incorrect flag value after wait_for"; // stopFlag should still be false
    // // should reflect stopFlag before thread changes to (!false)
    EXPECT_TRUE(result) << "Incorrect result from wait_for"; // stopFlag returned should still be false, returned inverted
    EXPECT_TRUE(end - start < timeout * 1000 * 2) << "wait_for returned too soon";


    pthread_cancel(t.native_handle());
    t.join();
}

TEST(UtilsTests, UtilsConcat) {    
    DesktopLogger::logd(TAG, "utils_concat");

    // test ints
    const std::vector<int> v1{1, 2, 1};
    const std::vector<int> v2{4, 5, 1};
    const std::vector<int> v3{1, 2, 1, 4, 5, 1};
    auto v4 = utils::concat(v1, v2, false);

    DesktopLogger::logd(TAG, "v1: [%s]", utils::join(v1, std::string(", ")).c_str());
    DesktopLogger::logd(TAG, "v2: [%s]", utils::join(v2, ", ").c_str());
    DesktopLogger::logd(TAG, "v3: [%s]", utils::join(v3, ", ").c_str());
    DesktopLogger::logd(TAG, "v4: [%s]", utils::join(v4, ", ").c_str());
    ASSERT_TRUE(std::equal(v3.begin(), v3.end(), v4.begin())) << "incorrect int concat result";


    const std::vector<int> v5{1, 2, 4, 5};
    auto v6 = utils::concat(v1, v2, true);

    DesktopLogger::logd(TAG, "v1: [%s]", utils::join(v1, ", ").c_str());
    DesktopLogger::logd(TAG, "v2: [%s]", utils::join(v2, ", ").c_str());
    DesktopLogger::logd(TAG, "v3: [%s]", utils::join(v3, ", ").c_str());
    DesktopLogger::logd(TAG, "v4: [%s]", utils::join(v4, ", ").c_str());
    EXPECT_TRUE(std::equal(v6.begin(), v6.end(), v5.begin())) << "incorrect int unique concat result";

    // test std::string
    std::vector<std::string> v7{"1", "2", "3"};
    std::vector<std::string> v8{"4", "5", "6"};
    std::vector<std::string> v9{"1", "2", "3", "4", "5", "6"};
    auto v10 = utils::concat(v7, v8);
    // compare std::vectors of strings
    bool equal = std::equal(v10.begin(), v10.end(), v9.begin());

    EXPECT_TRUE(equal) << "incorrect string concat result";
}

TEST(UtilsTests, UtilsSplit) {
    DesktopLogger::logd(TAG, "utils_split");

    std::string str{"test1,test2,test3"};
    std::vector<std::string> v1{"test1", "test2", "test3"};
    auto v2 = utils::split(str, ',');

    EXPECT_TRUE(std::equal(v1.begin(), v1.end(), v2.begin())) << "incorrect split result";
}

TEST(UtilsTest, UtilsJoin) {
    DesktopLogger::logd(TAG, "utils_join");

    std::vector<std::string> v1{"test1", "test2", "test3"};
    std::string str{"test1,test2,test3"};
    auto str2 = utils::join(v1, ",");

    EXPECT_EQ(str, str2) << "incorrect join result";

    std::vector<int> v2{1, 2, 3};
    std::string str3{"1,2,3"};
    auto str4 = utils::join(v2, ",");

    EXPECT_EQ(str3, str4) << "incorrect join result";
}

TEST(UtilsTests, UtilsTrimClean) {
    DesktopLogger::logd(TAG, "utils_wait_for_flag");

    std::string str{"test"}; //
    std::string str2{" test"};
    std::string str3{"test "};
    std::string str4{" test "};
    std::string str5{"test test"}; //
    std::string str6{" test test "};
    std::string str7{" test test"};
    std::string str8{"test test "};
    std::string str9{" test test\n"};
    std::string str10{"\ttest test\n"};
    std::string str11{"\ttest test\n "};
    std::string str12{"test test\n test"}; //
    std::string str13{"\ttest test\n test"};
    std::string str14{"\ttest test\n test "};
    std::string str15{"\ttest test\n test\t"};
    std::string str16{"\ttest test\n test\t "};
    std::string str17{"test test\n test\t test"}; //
    std::string str18{"\ttest test\n test\t test"};
    std::string str19{""}; //
    std::string str20{" "};
    std::string str21{"\n"};
    std::string str22{"\t"};

    EXPECT_EQ(str, utils::trim_clean(str)) << "incorrect trim_clean result";
    EXPECT_EQ(str, utils::trim_clean(str2)) << "incorrect trim_clean result";
    EXPECT_EQ(str, utils::trim_clean(str3)) << "incorrect trim_clean result";
    EXPECT_EQ(str, utils::trim_clean(str4)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str5)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str6)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str7)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str8)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str9)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str10)) << "incorrect trim_clean result";
    EXPECT_EQ(str5, utils::trim_clean(str11)) << "incorrect trim_clean result";
    EXPECT_EQ(str12, utils::trim_clean(str12)) << "incorrect trim_clean result";
    EXPECT_EQ(str12, utils::trim_clean(str13)) << "incorrect trim_clean result";
    EXPECT_EQ(str12, utils::trim_clean(str14)) << "incorrect trim_clean result";
    EXPECT_EQ(str12, utils::trim_clean(str15)) << "incorrect trim_clean result";
    EXPECT_EQ(str12, utils::trim_clean(str16)) << "incorrect trim_clean result";
    EXPECT_EQ(str17, utils::trim_clean(str17)) << "incorrect trim_clean result";
    EXPECT_EQ(str17, utils::trim_clean(str18)) << "incorrect trim_clean result";
    EXPECT_EQ(str19, utils::trim_clean(str19)) << "incorrect trim_clean result";
    EXPECT_EQ(str19, utils::trim_clean(str20)) << "incorrect trim_clean result";
    EXPECT_EQ(str19, utils::trim_clean(str21)) << "incorrect trim_clean result";
    EXPECT_EQ(str19, utils::trim_clean(str22)) << "incorrect trim_clean result";
}