#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <vector>
struct PlayInfo {
    std::string path;
    double duration{0.0};
    std::string title;
    std::optional<std::vector<std::string>> artists;

    nlohmann::json toJson();
    static PlayInfo fromJson(Yvar json);
};