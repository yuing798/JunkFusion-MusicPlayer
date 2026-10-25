#include "./PlayInfo.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/Yvar.hpp"

Yvar PlayInfo::toJson() const {
    Yvar obj = Yvar::object();
    obj.set(SongInfoMacro::path, path);
    obj.set(SongInfoMacro::artists, Yvar::fromOptional(artists));
    obj.set(SongInfoMacro::duration, duration);
    obj.set(SongInfoMacro::title, title);
    return obj;
}
PlayInfo PlayInfo::fromJson(const Yvar& json) {
    PlayInfo info{};
    info.path = json.read(SongInfoMacro::path).toString();
    info.artists = json.read(SongInfoMacro::artists).getOpt<std::vector<std::string>>();
    info.duration = json.read(SongInfoMacro::duration).toDouble();
    info.title = json.read(SongInfoMacro::title).toString();
    return info;
}
