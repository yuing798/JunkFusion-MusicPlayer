#include "./SongInfo.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/convertUtils.hpp"

juce::var SongInfo::toJson() {
    juce::var obj{new juce::DynamicObject()};

    auto ptr{obj.getDynamicObject()};

    // ── 0. 主键 ──
    ptr->setProperty(SongInfoMacro::songId, songId);

    // ── 1. 文件信息 ──
    ptr->setProperty(SongInfoMacro::duration, duration);

    // ── 2. 标签信息 ──
    ptr->setProperty(SongInfoMacro::title, title);
    ptr->setProperty(SongInfoMacro::artists, ConvertUtils::stringArray2ArrayVar(artists));
    ptr->setProperty(SongInfoMacro::album, ConvertUtils::juceStringToVar(album));
    ptr->setProperty(SongInfoMacro::albumArtist, ConvertUtils::juceStringToVar(albumArtist));
    ptr->setProperty(SongInfoMacro::genre, ConvertUtils::juceStringToVar(genre));
    ptr->setProperty(SongInfoMacro::trackNumber, ConvertUtils::optionalIntToVar(trackNumber));
    ptr->setProperty(SongInfoMacro::discNumber, ConvertUtils::optionalIntToVar(discNumber));
    ptr->setProperty(SongInfoMacro::year, ConvertUtils::optionalIntToVar(year));
    ptr->setProperty(SongInfoMacro::composer, ConvertUtils::juceStringToVar(composer));

    // ── 3. FFmpeg 解码层 ──
    ptr->setProperty(SongInfoMacro::bitRate, bitRate);
    ptr->setProperty(SongInfoMacro::sampleRate, sampleRate);
    ptr->setProperty(SongInfoMacro::channelLayout, channelLayout);
    ptr->setProperty(SongInfoMacro::bitDepth, bitDepth);
    ptr->setProperty(SongInfoMacro::codecName, ConvertUtils::juceStringToVar(codecName));

    // ── 4. AI 分析 ──
    ptr->setProperty(SongInfoMacro::aiGenre, ConvertUtils::juceStringToVar(aiGenre));
    ptr->setProperty(SongInfoMacro::bpm, ConvertUtils::optionalIntToVar(bpm));
    ptr->setProperty(SongInfoMacro::key, ConvertUtils::juceStringToVar(key));

    // ── 5. 用户信息 ──
    ptr->setProperty(SongInfoMacro::isMyLike, isMyLike);
    ptr->setProperty(SongInfoMacro::comment, ConvertUtils::juceStringToVar(comment));
    ptr->setProperty(SongInfoMacro::playNum, playNum);

    ptr->setProperty(SongInfoMacro::hash, ConvertUtils::juceStringToVar(hash));

    return obj;
} // 将songInfo转化为var，才能推送给前端