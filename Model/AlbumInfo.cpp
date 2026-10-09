#include "./AlbumInfo.hpp"
#include "Macro/AlbumInfoMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "juce_core/juce_core.h"

juce::var AlbumInfo::albumViewtoJson() {
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::alBumId, albumId);
    obj.getDynamicObject()->setProperty(
        AlbumInfoMacro::albumArtists,
        ConvertUtils::stringArray2ArrayVar(albumArtists)
    );
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::title, title);
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::hash, hash);
    return obj;
}
juce::var AlbumInfo::toJson() {
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::alBumId, albumId);
    obj.getDynamicObject()->setProperty(
        AlbumInfoMacro::albumArtists,
        ConvertUtils::stringArray2ArrayVar(albumArtists)
    );
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::title, title);
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::hash, hash);
    obj.getDynamicObject()->setProperty(
        AlbumInfoMacro::songsLists,
        ConvertUtils::vectorToArrayVar(songsList)
    );
    return obj;
}