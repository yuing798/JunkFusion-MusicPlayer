#include "./Yvar.hpp"
#include "juce_core/juce_core.h"

Yvar Yvar::read(const char* key) const {
    if (!value.isObject()) {
        return {};
    }
    if (!value.hasProperty(key)) {
        return {};
    }
    return value[key];
}
Yvar Yvar::read(int index) const {
    if (!value.isArray()) {
        return {};
    }
    if (index < 0 || index >= value.size()) {
        return {};
    }
    return value[index];
}
double Yvar::toDouble() const {
    if (value.isDouble()) {
        return static_cast<double>(value);
    } else {
        return 0.0;
    }
}
int Yvar::toInt() const {
    if (value.isInt()) {
        return static_cast<int>(value);
    } else {
        return -1;
    }
}

float Yvar::toFloat32() const {
    if (value.isDouble()) {
        return static_cast<float>(value);
    } else {
        return 0.0f;
    }
}

juce::String Yvar::toString() {

    if (value.isArray() || value.isObject()) {
        return juce::JSON::toString(value);
    } else {
        return value.toString();
    }
}
juce::StringArray Yvar::toStringArray() const {
    if (!value.isArray()) return juce::StringArray{};
    juce::StringArray arr;
    for (int i = 0; i < value.size(); i++) {
        arr.add(read(i).toString());
    }
    return arr;
}
bool Yvar::toBool() const {
    if (!value.isBool()) return 0;
    return static_cast<bool>(value);
}