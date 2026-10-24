#include "./playlistInfo.hpp"
#include "Macro/PlaylistMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>

juce::var PlaylistInfo::toBasicJson() {
    juce::var obj{new juce::DynamicObject()};
    auto ptr{obj.getDynamicObject()};
    ptr->setProperty(PlaylistMacro::id, id);
    ptr->setProperty(PlaylistMacro::title, title);
    ptr->setProperty(PlaylistMacro::hash, ConvertUtils::optValueToVar(hash));
    ptr->setProperty(PlaylistMacro::createTime, createTime);
    return obj;
}
juce::var PlaylistInfo::toDetailJson() {
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(
        PlaylistMacro::songList,
        ConvertUtils::vectorToArrayVar(songIdList)
    );
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