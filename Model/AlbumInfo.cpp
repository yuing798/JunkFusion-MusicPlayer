#include "./AlbumInfo.hpp"
#include "Macro/AlbumInfoMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>

juce::var AlbumInfo::toBasicJson() {
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::albumId, albumId);
    obj.getDynamicObject()->setProperty(
        AlbumInfoMacro::albumArtists,
        ConvertUtils::stringArray2ArrayVar(albumArtists)
    );
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::title, title);
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::hash, ConvertUtils::optValueToVar(hash));
    return obj;
}
juce::var AlbumInfo::toCompleteJson() {
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::albumId, albumId);
    obj.getDynamicObject()->setProperty(
        AlbumInfoMacro::albumArtists,
        ConvertUtils::stringArray2ArrayVar(albumArtists)
    );
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::title, title);
    obj.getDynamicObject()->setProperty(AlbumInfoMacro::hash, ConvertUtils::optValueToVar(hash));
    obj.getDynamicObject()->setProperty(
        AlbumInfoMacro::songsLists,
        ConvertUtils::vectorToArrayVar(songsList)
    );
    return obj;
}
