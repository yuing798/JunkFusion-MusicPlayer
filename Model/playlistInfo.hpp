#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <optional>
#include <vector>
struct PlaylistInfo {
    int64_t id{0};
    std::string title;
    std::optional<std::string> hash;
    std::vector<int64_t> songIdList;
    std::string createTime;

    Yvar toBasicJson() const;
    Yvar toDetailJson() const;
    static PlaylistInfo fromCompleteJson(const Yvar& obj);
};
