#include "./SongInfo.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

juce::var SongInfo::toCompleteJson() {
    juce::var obj{new juce::DynamicObject()};

    auto ptr{obj.getDynamicObject()};

    // ── 0. 主键 ──
    ptr->setProperty(SongInfoMacro::songId, songId);

    // ── 1. 文件信息 ──
    ptr->setProperty(SongInfoMacro::duration, duration);

    ptr->setProperty(SongInfoMacro::title, title);
    ptr->setProperty(SongInfoMacro::artists, ConvertUtils::stringArray2ArrayVar(artists));
    ptr->setProperty(SongInfoMacro::album, ConvertUtils::optValueToVar(album));
    ptr->setProperty(SongInfoMacro::trackNumber, ConvertUtils::optValueToVar(trackNumber));
    ptr->setProperty(SongInfoMacro::discNumber, ConvertUtils::optValueToVar(discNumber));

    // ── 5. 用户信息 ──
    ptr->setProperty(SongInfoMacro::isMyLike, isMyLike);
    ptr->setProperty(SongInfoMacro::playNum, playNum);

    ptr->setProperty(SongInfoMacro::hash, ConvertUtils::optValueToVar(hash));

    // ── 3. FFmpeg 解码层 ──
    ptr->setProperty(SongInfoMacro::bitRate, bitRate);
    ptr->setProperty(SongInfoMacro::sampleRate, sampleRate);
    ptr->setProperty(SongInfoMacro::channelLayout, channelLayout);
    ptr->setProperty(SongInfoMacro::bitDepth, bitDepth);
    ptr->setProperty(SongInfoMacro::codecName, ConvertUtils::optValueToVar(codecName));

    // ── 4. AI 分析 ──
    ptr->setProperty(SongInfoMacro::bpm, ConvertUtils::optValueToVar(bpm));
    ptr->setProperty(SongInfoMacro::key, ConvertUtils::optValueToVar(key));

    ptr->setProperty(SongInfoMacro::timeDomainSpec, ConvertUtils::vectorToArrayVar(timeDomainSpec));

    return obj;
} // 将songInfo转化为var，才能推送给前端
juce::var SongInfo::toBasicJson() {
    juce::var obj{new juce::DynamicObject()};

    auto ptr{obj.getDynamicObject()};

    // ── 0. 主键 ──
    ptr->setProperty(SongInfoMacro::songId, songId);

    // ── 1. 文件信息 ──
    ptr->setProperty(SongInfoMacro::duration, duration);

    ptr->setProperty(SongInfoMacro::title, title);
    ptr->setProperty(SongInfoMacro::artists, ConvertUtils::stringArray2ArrayVar(artists));
    ptr->setProperty(SongInfoMacro::album, ConvertUtils::optValueToVar(album));
    ptr->setProperty(SongInfoMacro::trackNumber, ConvertUtils::optValueToVar(trackNumber));
    ptr->setProperty(SongInfoMacro::discNumber, ConvertUtils::optValueToVar(discNumber));

    // ── 5. 用户信息 ──
    ptr->setProperty(SongInfoMacro::isMyLike, isMyLike);
    ptr->setProperty(SongInfoMacro::playNum, playNum);

    ptr->setProperty(SongInfoMacro::hash, ConvertUtils::optValueToVar(hash));

    return obj;
}
