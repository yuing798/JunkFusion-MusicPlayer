#include "./AlbumInfo.hpp"
#include "Macro/AlbumInfoMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/dbUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>

Yvar AlbumInfo::toBasicJson() const {
    Yvar obj = Yvar::object();
    obj.set(AlbumInfoMacro::albumId, albumId);
    obj.set(AlbumInfoMacro::albumArtists, Yvar::fromOptional(albumArtists));
    obj.set(AlbumInfoMacro::title, title);
    obj.set(AlbumInfoMacro::hash, Yvar::fromOptional(hash));
    return obj;
}
Yvar AlbumInfo::toDetailJson() const {
    Yvar obj = Yvar::object();
    obj.set(AlbumInfoMacro::songsLists, songsList);
    return obj;
}
