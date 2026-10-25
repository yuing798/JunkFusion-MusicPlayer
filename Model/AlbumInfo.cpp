#include "./AlbumInfo.hpp"
#include "Macro/AlbumInfoMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <nlohmann/json_fwd.hpp>

nlohmann::json AlbumInfo::toBasicJson() {
    nlohmann::json obj;
    obj[AlbumInfoMacro::albumId] = albumId;
    obj[AlbumInfoMacro::albumArtists] = ConvertUtils::optValueToVar(albumArtists);
    obj[AlbumInfoMacro::title] = title;
    obj[AlbumInfoMacro::hash] = ConvertUtils::optValueToVar(hash);
    return obj;
}
nlohmann::json AlbumInfo::toDetailJson() {
    nlohmann::json obj;
    obj[AlbumInfoMacro::songsLists] = songsList;
    return obj;
}
