#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <optional>
#include <vector>
struct PlaylistInfo {
    int64_t id{0};
    juce::String title;
    std::optional<juce::String> hash;
    std::vector<int64_t> songIdList;

    juce::var toBasicJson();
    juce::var toCompleteJson();
    static PlaylistInfo fromJson(Yvar obj);
};