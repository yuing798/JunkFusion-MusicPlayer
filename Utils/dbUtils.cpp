#include "./dbUtils.hpp"

namespace DbUtils {
    std::string addPrefixAndIntoVectorString(const std::string& prefix, int num) {
        std::string res;
        res += "(";
        for (int i = 0; i < num; i++) {
            res += prefix + std::to_string(i);
            if (i != num - 1) {
                res += ",";
            }
        }
        res += ")";
        return res;
    }
} // namespace DbUtils