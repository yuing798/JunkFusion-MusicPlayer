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

juce::String Yvar::toString() {

    if (value.isArray() || value.isObject()) {
        return juce::JSON::toString(value);
    } else {
        return value.toString();
    }
}

// void Yvar::write(Yvar v) {
//     if (!value.isArray()) return;
//     value.getArray()->add(v.value);
// }
// void Yvar::write(const char* key, Yvar v) {
//     if (!value.isObject()) return;
//     value.getDynamicObject()->setProperty(key, v.value);
// }