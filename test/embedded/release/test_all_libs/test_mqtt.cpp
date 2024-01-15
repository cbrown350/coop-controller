#include <gtest/gtest.h>

#include "../../TestLoggerFixture.h"

#include "settings.h"

#include <MQTTController.h>
#include <utils.h>


inline static constexpr const char * TAG{"tmqtt"};

MQTTController mqtt{"mqtt", "test_coop"};
MQTTController mqtt2{"mqtt2", "test_coop2"};

class TestMQTT : public TestLoggerFixture { };

TEST_F(TestMQTT, TestMQTT2StartStop) {
    // GTEST_SKIP();
    Logger<>::logd(TAG, "test_mqtt2_data");

    mqtt2.init();
    mqtt2.setLastWill("test_coop2/status");

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

    std::this_thread::sleep_for(std::chrono::seconds(1));
    mqtt2.stopLoop();
}

