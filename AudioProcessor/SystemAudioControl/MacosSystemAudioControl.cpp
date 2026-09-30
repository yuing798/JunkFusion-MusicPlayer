#include "MacosSystemAudioControl.hpp"

// 本文件为 Objective-C++ 实现（CMake 中已为 .cpp 强制 -x objective-c++，或建议改名为 .mm）。
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>
#import <AVFoundation/AVFoundation.h>

#include <atomic>
#include <memory>

// 平台实现细节全部收敛进 Impl，避免在头文件暴露 Objective-C 头
struct MacosSystemAudioControl::Impl {
    // MPRemoteCommandCenter 的命令目标对象必须强引用，否则会被 ARC 提前释放
    id playTarget{nullptr};
    id pauseTarget{nullptr};
    id toggleTarget{nullptr};
    id nextTarget{nullptr};
    id previousTarget{nullptr};
    id seekTarget{nullptr};

    std::atomic<bool> initialized{false};
};

// 便捷构造 MPRemoteCommandHandler 目标；handler 在媒体按键到达时被系统调用
static id makeCommandTarget(MPRemoteCommand* command, void (^handler)(MPRemoteCommandEvent*)) {
    // FIXME(连接): 这里用 block 包装系统回调，block 内需自行调度到主线程；
    //             目前直接调用 handler，若回调线程不安全请在 handler 内再包一层 dispatch_async。
    return [command addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
        handler(event);
        return MPRemoteCommandHandlerStatusSuccess;
    }];
}

MacosSystemAudioControl::MacosSystemAudioControl() : mImpl(std::make_unique<Impl>()) {}

MacosSystemAudioControl::~MacosSystemAudioControl() {
    shutdown();
}

bool MacosSystemAudioControl::initialize() {
    if (mImpl->initialized.load()) {
        return true;
    }

    @autoreleasepool {
        MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];

        // 系统媒体键：播放
        mImpl->playTarget = makeCommandTarget(center.playCommand, ^(MPRemoteCommandEvent*) {
            // TODO(连接): 回调来自系统线程，建议 dispatch 到主线程后再触发 onPlay
            if (onPlay) { onPlay(); }
        });

        // 系统媒体键：暂停
        mImpl->pauseTarget = makeCommandTarget(center.pauseCommand, ^(MPRemoteCommandEvent*) {
            if (onPause) { onPause(); }
        });

        // 系统媒体键：播放/暂停切换（耳机上的单键常走这里）
        mImpl->toggleTarget = makeCommandTarget(center.togglePlayPauseCommand, ^(MPRemoteCommandEvent*) {
            // FIXME(连接): 基类没有提供“当前是否正在播放”的状态查询，toggle 无法判断方向。
            //             请接入当前播放状态后再决定调用 onPlay 还是 onPause。
        });

        // 系统媒体键：下一首
        mImpl->nextTarget = makeCommandTarget(center.nextTrackCommand, ^(MPRemoteCommandEvent*) {
            if (onNext) { onNext(); }
        });

        // 系统媒体键：上一首
        mImpl->previousTarget = makeCommandTarget(center.previousTrackCommand, ^(MPRemoteCommandEvent*) {
            if (onPrevious) { onPrevious(); }
        });

        // 系统媒体键：拖动进度（控制中心 / 触摸板的进度条）
        mImpl->seekTarget = makeCommandTarget(center.changePlaybackPositionCommand, ^(MPRemoteCommandEvent* event) {
            if ([event isKindOfClass:[MPChangePlaybackPositionCommandEvent class]] && onSeek) {
                MPChangePlaybackPositionCommandEvent* seekEvent = (MPChangePlaybackPositionCommandEvent*)event;
                double seconds = seekEvent.positionTime;
                onSeek(seconds);
            }
        });

        // 默认启用常用命令（关闭不需要的命令，避免系统显示无关按钮）
        center.playCommand.enabled = YES;
        center.pauseCommand.enabled = YES;
        center.togglePlayPauseCommand.enabled = YES;
        center.nextTrackCommand.enabled = YES;
        center.previousTrackCommand.enabled = YES;
        center.changePlaybackPositionCommand.enabled = YES;

        // TODO(连接): 若需要封面图，这里可以启用 center.nextTrackCommand 之外的信息，
        //             并在 updateMetadata 里通过 MPMediaItemArtwork 设置 MPMediaItemPropertyArtwork。
    }

    mImpl->initialized.store(true);
    return true;
}

void MacosSystemAudioControl::updateMetadata(const MediaMetadata& metadata) {
    if (!mImpl->initialized.load()) {
        return;
    }

    @autoreleasepool {
        NSMutableDictionary* nowPlaying = [NSMutableDictionary dictionary];

        juce::String title = metadata.title;
        nowPlaying[MPMediaItemPropertyTitle] = title.toNSString();
        nowPlaying[MPMediaItemPropertyArtist] = metadata.artists.joinIntoString(", ").toNSString();

        // FIXME(连接): 封面图尚未接入——MediaMetadata 结构体目前没有 cover 字段，
        //             若要显示封面请扩展 MediaMetadata 增加封面数据/路径，再填充 MPMediaItemPropertyArtwork。

        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:nowPlaying];
    }
}

void MacosSystemAudioControl::updatePlaybackState(PlaybackState state) {
    if (!mImpl->initialized.load()) {
        return;
    }

    @autoreleasepool {
        NSMutableDictionary* info = [[[MPNowPlayingInfoCenter defaultCenter] nowPlayingInfo] mutableCopy];
        if (!info) {
            info = [NSMutableDictionary dictionary];
        }

        info[MPNowPlayingInfoPropertyPlaybackRate] = (state == PlaybackState::Play) ? @(1.0) : @(0.0);
        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:info];
    }
}

void MacosSystemAudioControl::shutdown() {
    if (!mImpl->initialized.exchange(false)) {
        return;
    }

    @autoreleasepool {
        MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];

        if (mImpl->playTarget) { [center.playCommand removeTarget:mImpl->playTarget]; }
        if (mImpl->pauseTarget) { [center.pauseCommand removeTarget:mImpl->pauseTarget]; }
        if (mImpl->toggleTarget) { [center.togglePlayPauseCommand removeTarget:mImpl->toggleTarget]; }
        if (mImpl->nextTarget) { [center.nextTrackCommand removeTarget:mImpl->nextTarget]; }
        if (mImpl->previousTarget) { [center.previousTrackCommand removeTarget:mImpl->previousTarget]; }
        if (mImpl->seekTarget) { [center.changePlaybackPositionCommand removeTarget:mImpl->seekTarget]; }

        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:nil];
    }

    mImpl->playTarget = nullptr;
    mImpl->pauseTarget = nullptr;
    mImpl->toggleTarget = nullptr;
    mImpl->nextTarget = nullptr;
    mImpl->previousTarget = nullptr;
    mImpl->seekTarget = nullptr;
}
