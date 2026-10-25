#include "./PlayInfo.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/convertUtils.hpp"
#include <nlohmann/json_fwd.hpp>

nlohmann::json PlayInfo::toJson() {
    nlohmann::json obj;
    obj[SongInfoMacro::path] = path;
    obj[SongInfoMacro::artists] = ConvertUtils::optValueToVar(artists);
    obj[SongInfoMacro::duration] = duration;
    obj[SongInfoMacro::title] = title;
    return obj;
}
PlayInfo PlayInfo::fromJson(Yvar json) {
    PlayInfo info{};
    info.path = json.read(SongInfoMacro::path).toString();
    info.artists = json.read(SongInfoMacro::artists).toStringArray();
    info.duration = json.read(SongInfoMacro::duration).toDouble();
    info.title = json.read(SongInfoMacro::title).toString();
    return info;
}