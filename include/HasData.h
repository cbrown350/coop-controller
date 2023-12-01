#ifndef HAS_DATA_H_
#define HAS_DATA_H_

template<typename T>
class HasData {
    public:
        virtual const T& getData() { return _data; };
    protected:
        T _data{};
};


#endif // HAS_DATA_H_