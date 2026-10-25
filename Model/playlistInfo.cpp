#include "./playlistInfo.hpp"
#include "Macro/PlaylistMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <nlohmann/json_fwd.hpp>

nlohmann::json PlaylistInfo::toBasicJson() {
    nlohmann::json obj;
    obj[PlaylistMacro::id] = id;
    obj[PlaylistMacro::title] = title;
    obj[PlaylistMacro::hash] = ConvertUtils::optValueToVar(hash);
    obj[PlaylistMacro::createTime] = createTime;
    return obj;
}
nlohmann::json PlaylistInfo::toDetailJson() {
    nlohmann::json obj;
    obj[PlaylistMacro::songList] = songIdList;
    return obj;
}
PlaylistInfo PlaylistInfo::fromCompleteJson(Yvar obj) {
    PlaylistInfo info{};
    info.title = obj.read(PlaylistMacro::title).toString();
    info.hash = obj.read(PlaylistMacro::hash).toOptStr();
    info.createTime = obj.read(PlaylistMacro::createTime).toString();
    info.songIdList = obj.read(PlaylistMacro::songList).toVector<int64_t>();
    return info;
}