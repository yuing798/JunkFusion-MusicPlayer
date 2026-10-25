#include "./SongInfo.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
}

nlohmann::json SongInfo::toDetailJson() {
    nlohmann::json obj;
    obj[SongInfoMacro::trackNumber] = ConvertUtils::optValueToVar(trackNumber);
    obj[SongInfoMacro::discNumber] = ConvertUtils::optValueToVar(discNumber);
    obj[SongInfoMacro::bitRate] = bitRate;
    obj[SongInfoMacro::sampleRate] = sampleRate;
    obj[SongInfoMacro::channelLayout] = channelLayout;
    obj[SongInfoMacro::bitDepth] = bitDepth;
    obj[SongInfoMacro::codecName] = ConvertUtils::optValueToVar(codecName);
    obj[SongInfoMacro::bpm] = ConvertUtils::optValueToVar(bpm);
    obj[SongInfoMacro::key] = ConvertUtils::optValueToVar(key);
    obj[SongInfoMacro::timeDomainSpec] = timeDomainSpec;

    return obj;
} // 将songInfo转化为var，才能推送给前端
nlohmann::json SongInfo::toBasicJson() {
    nlohmann::json obj;
    obj[SongInfoMacro::songId] = songId;
    obj[SongInfoMacro::duration] = duration;
    obj[SongInfoMacro::title] = title;
    obj[SongInfoMacro::artists] = ConvertUtils::optValueToVar(artists);
    obj[SongInfoMacro::album] = ConvertUtils::optValueToVar(album);
    obj[SongInfoMacro::isMyLike] = isMyLike;
    obj[SongInfoMacro::playNum] = playNum;
    obj[SongInfoMacro::hash] = ConvertUtils::optValueToVar(hash);

    return obj;
}
