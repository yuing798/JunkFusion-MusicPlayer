#pragma once

#include "juce_core/juce_core.h"
#include <cstdint>
#include <vector>
struct AlbumInfo {
    int64_t albumId{0};
    juce::StringArray albumArtists;
    juce::String title;
    juce::String hash;
    std::vector<int> songsList; // 这张专辑的歌曲列表

    // 将专辑视图的信息转为json，只需要艺术家，标题，hash三个属性
    juce::var albumViewtoJson();

    // 将所有信息转化为json
    juce::var toJson();
};