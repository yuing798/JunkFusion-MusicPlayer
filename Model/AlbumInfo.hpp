#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <optional>
#include <vector>
struct AlbumInfo {
    int64_t albumId{0};
    std::optional<juce::StringArray> albumArtists;
    juce::String title;
    std::optional<juce::String> hash;
    std::vector<int64_t> songsList; // 这张专辑的歌曲列表

    // 将专辑视图的信息转为json，只需要艺术家，标题，hash三个属性
    juce::var toBasicJson();

    // 将所有信息转化为json
    juce::var toDetailJson();
};