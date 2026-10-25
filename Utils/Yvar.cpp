#include "./Yvar.hpp"
#include "juce_core/juce_core.h"
#include <optional>
#include <string>
#include <vector>

Yvar Yvar::read(const char* key) const {
    if (!value.is_object()) {
        return {};
    }
    if (!value.contains(key)) {
        return {};
    }
    return value[key];
}
Yvar Yvar::read(int index) const {
    if (!value.is_array()) {
        return {};
    }
    if (index < 0 || index >= (int)value.size()) {
        return {};
    }
    return value[index];
}
double Yvar::toDouble() const {
    if (value.is_number_float()) {
        return static_cast<double>(value);
    } else {
        return 0.0;
    }
}
int Yvar::toInt() const {
    if (value.is_number_integer()) {
        return static_cast<int>(value);
    } else {
        return -1;
    }
}

std::string Yvar::toString() {

    if (value.is_array() || value.is_object()) {
        return value.dump();
    } else {
        return value;
    }
}

std::optional<std::string> Yvar::toOptStr() {
    if (value.is_string()) {
        return value;
    } else {
        return std::nullopt;
    }
}
std::vector<std::string> Yvar::toStringArray() const {
    if (!value.is_array()) return std::vector<std::string>{};
    std::vector<std::string> arr;
    for (int i = 0; i < (int)value.size(); i++) {
        arr.push_back(read(i).toString());
    }
    return arr;
}
bool Yvar::toBool() const {
    if (!value.is_boolean()) return 0;
    return static_cast<bool>(value);
}