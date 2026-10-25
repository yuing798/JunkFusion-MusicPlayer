#include "./playlistInfo.hpp"
#include "Macro/PlaylistMacro.hpp"
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>

Yvar PlaylistInfo::toBasicJson() const {
    Yvar obj = Yvar::object();
    obj.set(PlaylistMacro::id, id);
    obj.set(PlaylistMacro::title, title);
    obj.set(PlaylistMacro::hash, Yvar::fromOptional(hash));
    obj.set(PlaylistMacro::createTime, createTime);
    return obj;
}
Yvar PlaylistInfo::toDetailJson() const {
    Yvar obj = Yvar::object();
    obj.set(PlaylistMacro::songList, songIdList);
    return obj;
}
PlaylistInfo PlaylistInfo::fromCompleteJson(const Yvar& obj) {
    PlaylistInfo info{};
    info.title = obj.read(PlaylistMacro::title).toString();
    info.hash = obj.read(PlaylistMacro::hash).toOptStr();
    info.createTime = obj.read(PlaylistMacro::createTime).toString();
    info.songIdList = obj.read(PlaylistMacro::songList).toVector<int64_t>();
    return info;
}
