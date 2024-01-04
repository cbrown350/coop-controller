#include <gtest/gtest.h>

#include "../../TestLoggerFixture.h"

#include "settings.h"

#include <MQTTController.h>
#include <utils.h>


inline static constexpr const char * TAG{"tmqtt"};

MQTTController mqtt{"mqtt", "test_coop"};
MQTTController mqtt2{"mqtt2", "test_coop2"};

class TestMQTT : public TestLoggerFixture { };


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


TEST_F(TestMQTT, TestMQTT2Data) {
    // GTEST_SKIP();
    Logger<>::logd(TAG, "test_mqtt2_data");

    TestMQTTHasData test_data2{"test_data2"};
    mqtt2.init();
    mqtt2.setLastWill("test_coop2/status");
    mqtt2.registerHasDataItem(&test_data2);

    utils::printDataDebug("1- mqtt2", mqtt2.getData());

    mqtt2.startLoop();

    utils::printDataDebug("2- mqtt2", mqtt2.getData());

    [[maybe_unused]] bool b1 = mqtt2.set(MQTTController::MQTT_USER, "test_user");
    [[maybe_unused]] bool b2 = mqtt2.set(MQTTController::MQTT_PASSWORD, "test_password");
    [[maybe_unused]] bool b3 = mqtt2.set(MQTTController::MQTT_SERVER, "test_server");
    [[maybe_unused]] bool b4 = mqtt2.set(MQTTController::MQTT_PORT, "1900");

    utils::printDataDebug("3- mqtt2", mqtt2.getData());

    mqtt2.stopLoop();

    utils::printDataDebug("4- mqtt2", mqtt2.getData());

    mqtt2.startLoop();

    utils::printDataDebug("5- mqtt2", mqtt2.getData());

    std::this_thread::sleep_for(std::chrono::seconds(10));
    mqtt2.stopLoop();
}
