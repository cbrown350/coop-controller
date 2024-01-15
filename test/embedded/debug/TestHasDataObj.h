#include <HasData.h>

#include <Logger.h>
#include <string>

class TestHasDataObj : public HasData<> {
public:
    explicit TestHasDataObj(const std::string &instanceID) : HasData(instanceID) {}

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