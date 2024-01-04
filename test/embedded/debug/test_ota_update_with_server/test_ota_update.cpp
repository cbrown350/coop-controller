#include <gtest/gtest.h>
#include <thread>

#include "../../system_setup_test.h"
#include "../../TestLoggerFixture.h"

#include <ota_update.h>

inline static constexpr const char * TAG{"totaup"};

class TestOTAUpdate : public TestLoggerFixture { };

TEST_F(TestOTAUpdate, TestOTAUpdateStartStop) { 
    // GTEST_SKIP();
    
    ota_update::startLoop(false);
    std::this_thread::sleep_for(std::chrono::seconds(15));

    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [startLoop]")) << "startLoop call failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [serverOtaHandle]")) << "serverOtaHandle call failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [devOtaHandle]")) << "devOtaHandle call failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [checkUrlForUpdate]")) << "checkUrlForUpdate call failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] OTA update accepting connections")) << "devOtaHandle ready failed";

    // Below checks require a server to be set up and available
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] SERVER_OTA_UPDATE_URL: ")) << "check url checkUrlForUpdate failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] New firmware version: ")) << "new version checkUrlForUpdate failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] New firmware url: ")) << "url checkUrlForUpdate failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] Current version: ")) << "version checkUrlForUpdate failed";
    
    ota_update::stopLoop();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [stopLoop]")) << "stopLoop call failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [devOtaHandle] terminating")) << "devOTALoopMutex stopLoop failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] [serverOtaHandle] terminating")) << "serverOTALoopThread stopLoop failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] devOTALoopMutex stopped")) << "devOTALoopMutex stopLoop failed";
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[otaup] serverOTALoopThread stopped")) << "serverOTALoopThread stopLoop failed";       
}

