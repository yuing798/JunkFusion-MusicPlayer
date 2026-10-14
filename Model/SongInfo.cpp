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

juce::var SongInfo::toDetailJson() {
    juce::var obj{new juce::DynamicObject()};

    auto ptr{obj.getDynamicObject()};

    // ── 0. 主键 ──
    ptr->setProperty(SongInfoMacro::songId, songId);

    // ── 1. 文件信息 ──
    ptr->setProperty(SongInfoMacro::duration, duration);

    // ── 2. 标签信息 ──
    ptr->setProperty(SongInfoMacro::title, title);
    ptr->setProperty(SongInfoMacro::artists, ConvertUtils::stringArray2ArrayVar(artists));
    ptr->setProperty(SongInfoMacro::album, ConvertUtils::optValueToVar(album));
    ptr->setProperty(SongInfoMacro::trackNumber, ConvertUtils::optValueToVar(trackNumber));
    ptr->setProperty(SongInfoMacro::discNumber, ConvertUtils::optValueToVar(discNumber));

    // ── 3. FFmpeg 解码层 ──
    ptr->setProperty(SongInfoMacro::bitRate, bitRate);
    ptr->setProperty(SongInfoMacro::sampleRate, sampleRate);
    ptr->setProperty(SongInfoMacro::channelLayout, channelLayout);
    ptr->setProperty(SongInfoMacro::bitDepth, bitDepth);
    ptr->setProperty(SongInfoMacro::codecName, ConvertUtils::optValueToVar(codecName));

    // ── 4. AI 分析 ──
    ptr->setProperty(SongInfoMacro::bpm, ConvertUtils::optValueToVar(bpm));
    ptr->setProperty(SongInfoMacro::key, ConvertUtils::optValueToVar(key));

    // ── 5. 用户信息 ──
    ptr->setProperty(SongInfoMacro::isMyLike, isMyLike);
    ptr->setProperty(SongInfoMacro::playNum, playNum);

    ptr->setProperty(SongInfoMacro::hash, ConvertUtils::optValueToVar(hash));
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

    // ── 5. 用户信息 ──
    ptr->setProperty(SongInfoMacro::isMyLike, isMyLike);
    ptr->setProperty(SongInfoMacro::playNum, playNum);

    ptr->setProperty(SongInfoMacro::hash, ConvertUtils::optValueToVar(hash));

    return obj;
}
SongInfo SongInfo::getBasicInfoFromSql(SQLite::Statement& query) {
    SongInfo info{};

    info.songId = query.getColumn("songId").getInt64();
    info.duration = query.getColumn("duration").getDouble();

    // ── 标签 ──
    info.title = query.getColumn("title").getString();
    info.artists = DbUtils::getOptCol<juce::StringArray>(query, "artists");
    info.album = DbUtils::getOptCol<juce::String>(query, "album");

    // ── 用户信息 ──
    info.isMyLike = query.getColumn("isMyLike").getInt() != 0;
    info.playNum = query.getColumn("playNum").getInt();
    info.hash = DbUtils::getOptCol<juce::String>(query, "hash");
    return info;
}
SongInfo SongInfo::getDetailInfoFromSql(SQLite::Statement& query) {
    SongInfo info{};

    info.songId = query.getColumn("songId").getInt64();
    info.duration = query.getColumn("duration").getDouble();

    // ── 标签 ──
    info.title = query.getColumn("title").getString();
    info.artists = DbUtils::getOptCol<juce::StringArray>(query, "artists");
    info.album = DbUtils::getOptCol<juce::String>(query, "album");
    info.trackNumber = DbUtils::getOptCol<int>(query, "trackNumber");
    info.discNumber = DbUtils::getOptCol<int>(query, "discNumber");

    // ── FFmpeg 解码层 ──
    info.bitRate = query.getColumn("bitRate").getInt64();
    info.bitDepth = query.getColumn("bitDepth").getInt();
    info.sampleRate = query.getColumn("sampleRate").getInt();

    auto channelLayoutMask = query.getColumn("channelLayoutMask").getInt64();
    if (channelLayoutMask != 0) {

        AVChannelLayout layout;
        av_channel_layout_from_mask(&layout, channelLayoutMask);
        char layoutBuffer[64] = {0};
        av_channel_layout_describe(&layout, layoutBuffer, sizeof(layoutBuffer));
        info.channelLayout = juce::String(layoutBuffer);
    }
    info.codecName = DbUtils::getOptCol<juce::String>(query, "codecName");

    // ── AI 分析 ──
    info.bpm = DbUtils::getOptCol<int>(query, "bpm");
    info.key = DbUtils::getOptCol<juce::String>(query, "key");

    // ── 用户信息 ──
    info.isMyLike = query.getColumn("isMyLike").getInt() != 0;
    info.playNum = query.getColumn("playNum").getInt();
    info.hash = DbUtils::getOptCol<juce::String>(query, "hash");
    info.timeDomainSpec = DbUtils::getBlob<double>(query, ":timeDomainSpec");
    return info;
}