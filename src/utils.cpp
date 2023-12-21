#include "utils.h"

#include <string>
#include <vector>

namespace utils {
    std::vector<std::string> split(const std::string_view view, char i) {
        std::vector<std::string> result;
        size_t pos = 0;
        while(true) {
            auto next = view.find(i, pos);
            if(next == std::string_view::npos) {
                result.emplace_back(view.substr(pos));
                break;
            }
            result.emplace_back(view.substr(pos, next - pos));
            pos = next + 1;
        }
        return result;
    }
} // namespace utils
