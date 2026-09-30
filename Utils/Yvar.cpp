#include "./Yvar.hpp"

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