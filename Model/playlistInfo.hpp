#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <vector>
struct PlaylistInfo {
    int64_t id{0};
    std::string title;
    std::optional<std::string> hash;
    std::vector<int64_t> songIdList;
    std::string createTime;

    nlohmann::json toBasicJson();
    nlohmann::json toDetailJson();
    static PlaylistInfo fromCompleteJson(Yvar obj);
};