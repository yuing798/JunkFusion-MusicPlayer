#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <optional>
struct PlayInfo {
    juce::String path;
    double duration{0.0};
    juce::String title;
    std::optional<juce::StringArray> artists;

    juce::var toJson();
    static PlayInfo fromJson(Yvar json);
};