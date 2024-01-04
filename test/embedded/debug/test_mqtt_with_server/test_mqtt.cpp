#include <gtest/gtest.h>

#include "../../TestLoggerFixture.h"

#include "settings.h"

#include <MQTTController.h>
#include <utils.h>


inline static constexpr const char * TAG{"tmqtt"};

MQTTController mqtt{"mqtt", "test_coop"};
MQTTController mqtt2{"mqtt2", "test_coop2"};

class TestMQTT : public TestLoggerFixture { };

TEST_F(TestMQTT, TestMQTTConnect) {    
    // GTEST_SKIP();
    Logger<>::logd(TAG, "test_mqtt_connect");     
    mqtt.init();
    mqtt.setLastWill("test_coop/status");
    mqtt.startLoop();
    std::this_thread::sleep_for(std::chrono::seconds(4));
    log->printLogs();
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[mqtt ] [startLoop] MQTT (0) startLoop")) << "startLoop call failed";
    
    mqtt.stopLoop();
    EXPECT_TRUE(log->checkMatchesLogEntryOrBeforeMsg(0, "[mqtt ] [stopLoop] MQTT (0) stopLoop")) << "stopLoop call failed";
    
}


class TestMQTTHasData : public HasData<> {
public:
    explicit TestMQTTHasData(const std::string &instanceID) : HasData(instanceID) {}

    [[nodiscard]] std::vector<std::string> getKeys() const override { return {"test_key1", "test_key2"}; }

    std::string test_key1 = "test_value1";
    std::string test_key2 = "test_value2";    

    [[nodiscard]] std::string getWithOptLock(const std::string &key, bool noLock) const override {
        if (key == "test_key1") {
            Logger::logd(TAG, "getWithOptLock test_key1");
            return test_key1;
        }
        if (key == "test_key2") {
            Logger::logd(TAG, "getWithOptLock test_key2");
            return test_key2;
        }
        return HasData::EMPTY_VALUE;
    }

    [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, const bool noLock,
                                                const bool doObjUpdate) override {
        if (key == "test_key1") {
            Logger::logd(TAG, "setWithOptLockAndUpdate test_key1, value %s", value.c_str());
            test_key1 = value;
            return true;
        }
        if (key == "test_key2") {
            Logger::logd(TAG, "setWithOptLockAndUpdate test_key2, value %s", value.c_str());
            test_key2 = value;
            return true;
        }
        return false;
    }
};

