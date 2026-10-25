#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
struct AlbumInfo {
    int64_t albumId{0};
    std::optional<std::vector<std::string>> albumArtists;
    std::string title;
    std::optional<std::string> hash;
    std::vector<int64_t> songsList; // 这张专辑的歌曲列表

    // 将专辑视图的信息转为json，只需要艺术家，标题，hash三个属性
    Yvar toBasicJson() const;

    // 将所有信息转化为json
    Yvar toDetailJson() const;
};
