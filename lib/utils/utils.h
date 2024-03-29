#ifndef UTILS_H_
#define UTILS_H_

#include <Logger.h>

#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <algorithm>
// #include <ranges>


namespace utils {
    inline static constexpr const char * const TAG{"utils"};

    template<class Duration>
    [[nodiscard]] bool inline wait_for(Duration duration, std::mutex &m, std::condition_variable &c, const bool &stop) {
        std::unique_lock<std::mutex> l(m);
        return !c.wait_for(l, duration, [&stop]() { return stop; });
    }

    // Attr: https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    template<typename ... Args>
    [[nodiscard]] inline std::string string_format(const std::string& format, Args ... args ) {
        int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
        if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
        auto size = static_cast<size_t>( size_s );
        std::unique_ptr<char[]> buf( new char[ size ] );
        std::snprintf( buf.get(), size, format.c_str(), args ... );
        return {buf.get(), &buf[size - 1]}; // We don't want the '\0' inside
    }

    // https://stackoverflow.com/questions/65195841/hash-function-to-switch-on-a-string
    [[nodiscard]] inline constexpr uint64_t hashstr(const char* s, size_t index = 0) {
        return (s /* + index */) == nullptr || s[index] == '\0' ? 55 : hashstr(s, index + 1) * 33 + (unsigned char)(s[index]);
    }

    [[nodiscard]] std::vector<std::string> split(std::string_view view, char i);

    template<typename C> //,
            // typename = std::enable_if_t<std::ranges::common_range<C>>>
    [[nodiscard]] inline std::string join(const C &container, const std::string &separator) {
        std::string result;
        for(auto it = container.begin(); it != container.end(); ++it) {
            if constexpr (std::is_same<typename C::value_type, std::string>::value)
                result += *it;
            else
                result += std::to_string(*it);
            if(it != container.end() - 1)
                result += separator;
        }
        return result;
    }

    template<typename T>
    [[nodiscard]] inline std::vector<T> concat(const std::vector<T>& v1, const std::vector<T>& v2, const bool makeUnique = false){
        if(makeUnique) {
            std::vector<T> new_v;
            new_v.reserve(v1.size() + v2.size());
            for(const auto &v: v1) {
                if(std::find(new_v.begin(), new_v.end(), v) != new_v.end())
                    continue;
                new_v.emplace_back(v);
            }
            for(const auto &v: v2) {
                if(std::find(new_v.begin(), new_v.end(), v) != new_v.end())
                    continue;
                new_v.emplace_back(v);
            }
            return new_v;
        } else {
            std::vector<T> new_v(std::begin(v1), std::end(v1));
            new_v.insert(std::end(new_v), std::begin(v2), std::end(v2));
            return new_v;
        }
    }

    [[nodiscard]] inline bool isPositiveNumber(const std::string& s) {
        return !s.empty() && s.at(0) != '-' && std::find_if(s.begin(),
                                          s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
    }

    inline void printDataDebug(const std::string &name, const std::map<std::string, std::string> &data) {
        Logger<>::logd(TAG, "%s: {", name.c_str());
        for (const auto &d: data)
            Logger<>::logd(TAG, "    -> key: %s = value: %s", d.first.c_str(), d.second.c_str());
        Logger<>::logd(TAG, "}");
    }

    // creates a copy of the string, removes carriage returns, spaces, newlines, and tabs on the ends
    // and returns the new string
    [[nodiscard]] inline std::string trim_clean(const std::string &s) {
        std::string str = s;
        for(auto it = str.begin(); it != str.end(); ++it) 
            if(*it == '\r' || *it == '\n' || *it == '\t' || *it == ' ')
                it = str.erase(it);
            else
                break;
        for(auto it = str.end()-1; it != str.begin(); --it) 
            if(*it == '\r' || *it == '\n' || *it == '\t' || *it == ' ')
                it = str.erase(it);
            else
                break;
        return str;
    }

} // namespace utils

#endif // UTILS_H_