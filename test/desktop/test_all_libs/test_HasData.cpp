
#include <gtest/gtest.h>

#include "../DesktopLoggerFixture.h"

#include <utils.h>

#include <string>
#include <vector>
#include <algorithm>

#include <HasData.h>


class TestHasDataObj : public HasData<> {
public:
    explicit TestHasDataObj(const char *name) : HasData(name) { }

    [[nodiscard]] std::string getNvsNamespace() const override { return "test_has_data"; }
    [[nodiscard]] std::vector<std::string> getNvsKeys() const override { return {"test_key1"}; }
    [[nodiscard]] std::vector<std::string> getReadOnlyKeys() const override { return {"test_key2"}; }
    [[nodiscard]] std::vector<std::string> getKeys() const override { return utils::concat(utils::concat({"test_key3"}, getReadOnlyKeys()), getNvsKeys(), true) ; }

    std::string test_key1 = "test_value1";
    std::string test_key2 = "test_value2";
    std::string test_key3 = "test_value3";

    mutable bool objUpdated = false;

    [[nodiscard]] std::string getWithOptLock(const std::string &key, bool noLock) const override {
        Logger::logd(TAG, "getWithOptLock %s", key.c_str());
        if (key == "test_key1") {
            Logger::logd(TAG, "getWithOptLock 'test_key1': '%s'", test_key1.c_str());
            return test_key1;
        }
        if (key == "test_key2") {
            Logger::logd(TAG, "getWithOptLock 'test_key2': '%s'", test_key2.c_str());
            return test_key2;
        }
        if (key == "test_key3") {
            Logger::logd(TAG, "getWithOptLock 'test_key3': '%s'", test_key3.c_str());
            return test_key3;
        }
        return HasData::EMPTY_VALUE;
    }

    [[nodiscard]] bool setWithOptLockAndUpdate(const std::string &key, const std::string &value, const bool noLock,
                                                const bool doObjUpdate) override {
        objUpdated = false;
        Logger::logd(TAG, "setWithOptLockAndUpdate %s, value %s", key.c_str(), value.c_str());
        const auto &keys = getKeys();
        if(std::find(keys.begin(), keys.end(), key) == keys.end()) {
            Logger::logd(TAG, "Keys: {%s}", utils::join(getKeys(), ", ").c_str());
            Logger::logw(TAG, "Key '%s' is not found", key.c_str());
            return false;
        }
        const auto &readOnlyKeys = getReadOnlyKeys();
        if(std::find(readOnlyKeys.begin(), readOnlyKeys.end(), key) != readOnlyKeys.end()) {
            Logger::logd(TAG, "Read-only keys: {%s}", utils::join(getReadOnlyKeys(), ", ").c_str());
            Logger::logw(TAG, "Key '%s' is read-only", key.c_str());
            return false;
        }

        static std::vector<std::string> keysToUpdateOnObj = {};
        bool updated = false;

        if (key == "test_key1") {
            Logger::logd(TAG, "setWithOptLockAndUpdate 'test_key1': '%s'", value.c_str());
            test_key1 = value;
            updated = true;
            keysToUpdateOnObj.push_back(key);
        }
        if (key == "test_key2") {
            Logger::logd(TAG, "setWithOptLockAndUpdate 'test_key2': '%s'", value.c_str());
            throw std::runtime_error("test exception, test_key2 is read-only");
            test_key2 = value;
            updated = true;
            keysToUpdateOnObj.push_back(key);
        }
        if (key == "test_key3") {
            Logger::logd(TAG, "setWithOptLockAndUpdate 'test_key3': '%s'", value.c_str());
            test_key3 = value;
            updated = true;
            keysToUpdateOnObj.push_back(key);
        }
        if(doObjUpdate && !keysToUpdateOnObj.empty()) {
            const bool objUpdated = updateObj(keysToUpdateOnObj, noLock);
            if(objUpdated) {
                Logger::logv(TAG, "Updated [%s]", utils::join(keysToUpdateOnObj, ", ").c_str());
                keysToUpdateOnObj.clear();
            }
            return objUpdated;
        }
        return updated;
    }

    [[nodiscard]] bool updateObj(const std::vector<std::string> &keys, const bool noLock) override {
        objUpdated = true;
        Logger::logd(TAG, "updateObj, %s [%s]", utils::join(keys, ", ").c_str(), noLock ? "noLock" : "lock");
        return HasData<>::updateObj(keys, noLock);
    }

};


class TestHasData : public DesktopLoggerFixture, 
                    public TestHasDataObj {
public:
    explicit TestHasData() : DesktopLoggerFixture(), 
                             TestHasDataObj(::testing::UnitTest::GetInstance()->current_test_info()->name()) { }
};

static constexpr const char * TAG{"thasdt"};

TEST_F(TestHasData, TestHasDataGetSet) {
    // GTEST_SKIP();
    Logger::logd(TAG, getInstanceID().c_str());

    utils::printDataDebug(getInstanceID(), getData());
    Logger::logd(TAG, "Keys: {%s}", utils::join(getKeys(), ", ").c_str());
    Logger::logd(TAG, "Read-only keys: {%s}", utils::join(getReadOnlyKeys(), ", ").c_str());

    EXPECT_EQ("test_value1", get("test_key1"));
    EXPECT_EQ("test_value2", get("test_key2"));

    EXPECT_TRUE(set("test_key1", "test_value1_new"));
    utils::printDataDebug(getInstanceID(), getData());

    objUpdated = false;
    EXPECT_FALSE(set("test_key2", "test_value2_new"));
    EXPECT_FALSE(objUpdated) << "updateObj was not called incorrectly on read-only key";
    utils::printDataDebug(getInstanceID(), getData());

    EXPECT_EQ("test_value1_new", get("test_key1"));
    EXPECT_NE("test_value2_new", get("test_key2"));

    objUpdated = false;
    EXPECT_TRUE(setData({{"test_key1", "test_value1_new2"},
                                   {"test_key2", "test_value2_new2"}}));
    utils::printDataDebug(getInstanceID(), getData());

    EXPECT_EQ("test_value1_new2", get("test_key1"));
    EXPECT_EQ("test_value2", get("test_key2"));

    objUpdated = false;
    EXPECT_TRUE(setData({{"test_key1", "test_value1_new3"},
                                    {"test_key2", "test_value2_new3"}}));
    utils::printDataDebug(getInstanceID(), getData());

    test_key2 = "test_value2_new4";

    EXPECT_EQ("test_value1_new3", get("test_key1"));
    EXPECT_EQ("test_value2_new4", get("test_key2"));

    objUpdated = false;
    EXPECT_FALSE(setData({{"test_key2", "test_value2_new5"}}));
    EXPECT_NE("test_value2_new5", get("test_key2"));
}

TEST_F(TestHasData, TestHasDataNVS) {
    // GTEST_SKIP();
    Logger::logd(TAG, getInstanceID().c_str());

    utils::printDataDebug(getInstanceID(), getData());
    Logger::logd(TAG, "Keys: {%s}", utils::join(getKeys(), ", ").c_str());
    Logger::logd(TAG, "Read-only keys: {%s}", utils::join(getReadOnlyKeys(), ", ").c_str());
    Logger::logd(TAG, "NVS keys: {%s}", utils::join(getNvsKeys(), ", ").c_str());
    
    EXPECT_EQ(EMPTY_VALUE, get("test_key_unknown"));

    EXPECT_EQ("test_value1", get("test_key1"));
    
    objUpdated = false;
    EXPECT_TRUE(setData({{"test_key1", "test_value1_new_nvs"},
                                   {"test_key3", "test_value3_new_nvs"}}));
    EXPECT_TRUE(objUpdated) << "updateObj was not called but should have been";
    utils::printDataDebug(getInstanceID(), getData());
    EXPECT_EQ("test_value1_new_nvs", get("test_key1"));   
    EXPECT_EQ("test_value3_new_nvs", get("test_key3"));     
    
    test_key3 = "test_value3_new_nvs_new_unsaved";
    EXPECT_TRUE(set("test_key3", "test_value3_new_unsaved"));
    EXPECT_TRUE(objUpdated) << "updateObj was not called but should have been";
    objUpdated = false;
    EXPECT_TRUE(loadNvsData());
    EXPECT_FALSE(objUpdated) << "updateObj was called when it shouldn't";
    utils::printDataDebug(getInstanceID(), getData());
    EXPECT_EQ("test_value1_new_nvs", get("test_key1"));   
    EXPECT_EQ("test_value3_new_unsaved", get("test_key3")) << "test_key3 should be unchanged since not NVS";   

    // create new object with same namesapce and load from NVS
    TestHasDataObj newObj{"TestHasDataNVS"};        
    utils::printDataDebug(newObj.getInstanceID()+"2", newObj.getData());      
    EXPECT_EQ("test_value1", newObj.get("test_key1"));   
    EXPECT_EQ("test_value3", newObj.get("test_key3"));  
    EXPECT_TRUE(newObj.loadNvsData());                                    
    utils::printDataDebug(newObj.getInstanceID()+"2", newObj.getData());    
    EXPECT_EQ("test_value1_new_nvs", newObj.get("test_key1"));   
    EXPECT_EQ("test_value3", newObj.get("test_key3"));  
}