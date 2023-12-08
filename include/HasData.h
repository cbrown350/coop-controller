#ifndef HAS_DATA_H_
#define HAS_DATA_H_

#include <mutex>
#include <map>
#include <string>

template<typename K = std::string, typename D = std::string, typename T = std::map<K, D>>
class HasData {
    public:
        virtual ~HasData() = default;
//        HasData() { this->setupDataVars(); };
//        virtual void setupDataVars() = 0;
//        virtual T getData() { std::scoped_lock l{_dataMutex}; return _data; };
//        virtual D getData(const K &key) { std::scoped_lock l{_dataMutex}; return _data[key]; };
//        virtual void setData(const T& data) { std::scoped_lock l{_dataMutex}; _data = data; };
//        virtual void setData(const K &key, const D &value) { std::scoped_lock l{_dataMutex}; _data[key] = value; };

        template<typename X>
        void setData(const K &key, const X &value) {
            std::scoped_lock l{_dataMutex};
            _data[key] = std::to_string(value);
        };
        virtual void setData(const K &key, const std::string &value) {
            std::scoped_lock l{_dataMutex};
            _data[key] = value;
        };
        virtual void setData(const K &key, const char *value) {
            std::scoped_lock l{_dataMutex};
            _data[key] = value;
        };
        virtual void setData(const K &key, const bool value) {
            std::scoped_lock l{_dataMutex};
            _data[key] = value ? "true" : "false";
        };

        virtual bool dataToBool(const K &key) {
            std::scoped_lock l{_dataMutex};
            return _data.find(key) != _data.end() &&
                    (_data[key].compare("true") == 0 || _data[key].compare("True") == 0);
        };
        virtual int dataToInt(const K &key) {
            std::scoped_lock l{_dataMutex};
            if(_data.find(key) == _data.end())
                return 0;
            return std::stoi(_data[key]);
        };
        virtual float dataToFloat(const K &key) {
            std::scoped_lock l{_dataMutex};
            if(_data.find(key) == _data.end())
                return 0;
            return std::stof(_data[key]);
        };
        virtual double dataToDouble(const K &key) {
            std::scoped_lock l{_dataMutex};
            if(_data.find(key) == _data.end())
                return 0;
            return std::stod(_data[key]);
        };
    protected:
        std::mutex _dataMutex;
        T _data{};
};


#endif // HAS_DATA_H_