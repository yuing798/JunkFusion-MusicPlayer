#include "./SongInfo.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

Yvar SongInfo::toDetailJson() const {
    Yvar obj = Yvar::object();
    obj.set(SongInfoMacro::trackNumber, Yvar::fromOptional(trackNumber));
    obj.set(SongInfoMacro::discNumber, Yvar::fromOptional(discNumber));
    obj.set(SongInfoMacro::bitRate, bitRate);
    obj.set(SongInfoMacro::sampleRate, sampleRate);
    obj.set(SongInfoMacro::channelLayout, channelLayout);
    obj.set(SongInfoMacro::bitDepth, bitDepth);
    obj.set(SongInfoMacro::codecName, Yvar::fromOptional(codecName));
    obj.set(SongInfoMacro::bpm, Yvar::fromOptional(bpm));
    obj.set(SongInfoMacro::key, Yvar::fromOptional(key));
    obj.set(SongInfoMacro::timeDomainSpec, timeDomainSpec);

    return obj;
} // 将songInfo转化为Yvar，才能推送给前端
Yvar SongInfo::toBasicJson() const {
    Yvar obj = Yvar::object();
    obj.set(SongInfoMacro::songId, songId);
    obj.set(SongInfoMacro::duration, duration);
    obj.set(SongInfoMacro::title, title);
    obj.set(SongInfoMacro::artists, Yvar::fromOptional(artists));
    obj.set(SongInfoMacro::album, Yvar::fromOptional(album));
    obj.set(SongInfoMacro::isMyLike, isMyLike);
    obj.set(SongInfoMacro::playNum, playNum);
    obj.set(SongInfoMacro::hash, Yvar::fromOptional(hash));

    return obj;
}
