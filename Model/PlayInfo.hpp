#pragma once

#include "Macro/SongInfoMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/convertUtils.hpp"
#include "juce_core/juce_core.h"
#include <optional>
#include <string>
struct PlayInfo {
    juce::String path;
    double duration{0.0};
    juce::String title;
    std::optional<juce::StringArray> artists;

    juce::var toJson();
    static PlayInfo fromJson(Yvar json);
};