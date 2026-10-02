#include "MacosSystemAudioControl.hpp"
#include "SystemAudioControl.hpp"
#include <atomic>
#include <memory>

// 本文件为 Objective-C++ 实现（CMake 中已为 .cpp 强制 -x objective-c++，或建议改名为 .mm）。
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <MediaPlayer/MediaPlayer.h>

// 平台实现细节全部收敛进 Impl，避免在头文件暴露 Objective-C 头
struct MacosSystemAudioControl::Impl {
    // MPRemoteCommandCenter 的命令目标对象必须强引用，否则会被 ARC 提前释放
    id playTarget{nullptr};
    id pauseTarget{nullptr};
    id toggleTarget{nullptr};
    id nextTarget{nullptr};
    id previousTarget{nullptr};
    id seekTarget{nullptr};
    id fastForwardTarget{nullptr};
    id rewindTarget{nullptr};
    // id stopTarget{nullptr};

    std::atomic<bool> initialized{false};
    PlaybackState currentPlaybackState{PlaybackState::Pause};
};

// 便捷构造 MPRemoteCommandHandler 目标；handler 在媒体按键到达时被系统调用
static id makeCommandTarget(MPRemoteCommand* command, void (^handler)(MPRemoteCommandEvent*)) {
    // FIXME(连接): 这里用 block 包装系统回调，block 内需自行调度到主线程；
    //             目前直接调用 handler，若回调线程不安全请在 handler 内再包一层 dispatch_async。
    return
        [command addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent* event) {
          handler(event);
          return MPRemoteCommandHandlerStatusSuccess;
        }];
}

MacosSystemAudioControl::MacosSystemAudioControl() : mImpl(std::make_unique<Impl>()) {}

MacosSystemAudioControl::~MacosSystemAudioControl() { shutdown(); }

bool MacosSystemAudioControl::initialize() {
    if (mImpl->initialized.load()) {
        return true;
    }

    @autoreleasepool {
        MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];

        // 系统媒体键：播放
        mImpl->playTarget = makeCommandTarget(center.playCommand, ^(MPRemoteCommandEvent*) {
          if (onPlay) {
              onPlay();
              mImpl->currentPlaybackState = PlaybackState::Play;
          }
        });

        // 系统媒体键：暂停
        mImpl->pauseTarget = makeCommandTarget(center.pauseCommand, ^(MPRemoteCommandEvent*) {
          if (onPause) {
              onPause();
              mImpl->currentPlaybackState = PlaybackState::Pause;
          }
        });

        // 系统媒体键：播放/暂停切换（耳机上的单键常走这里）
        mImpl->toggleTarget =
            makeCommandTarget(center.togglePlayPauseCommand, ^(MPRemoteCommandEvent*) {
              // 修复：不仅要改状态，还要真正调用回调
              if (mImpl->currentPlaybackState == PlaybackState::Play) {
                  if (onPause) onPause();
                  mImpl->currentPlaybackState = PlaybackState::Pause;
              } else {
                  if (onPlay) onPlay();
                  mImpl->currentPlaybackState = PlaybackState::Play;
              }
            });

        // 系统媒体键：下一首
        mImpl->nextTarget = makeCommandTarget(center.nextTrackCommand, ^(MPRemoteCommandEvent*) {
          if (onNext) {
              onNext();
          }
        });

        // 系统媒体键：上一首
        mImpl->previousTarget =
            makeCommandTarget(center.previousTrackCommand, ^(MPRemoteCommandEvent*) {
              if (onPrevious) {
                  onPrevious();
              }
            });

        // 系统媒体键：停止
        // mImpl->stopTarget = makeCommandTarget(center.stopCommand, ^(MPRemoteCommandEvent*) {
        //   if (onStop) {
        //       onStop();
        //   }
        //   // 停止后通常状态变为暂停或停止
        //   mImpl->currentPlaybackState = PlaybackState::Pause;
        // });

        // 系统媒体键：快进
        mImpl->fastForwardTarget =
            makeCommandTarget(center.skipForwardCommand, ^(MPRemoteCommandEvent*) {
              if (onFastForward) {
                  onFastForward();
              }
            });
        // 告诉系统快进按键每次跳转的秒数（比如10秒），不加这行按键可能没反应
        center.skipForwardCommand.preferredIntervals = @[ @(10.0) ];

        // 系统媒体键：快退
        mImpl->rewindTarget =
            makeCommandTarget(center.skipBackwardCommand, ^(MPRemoteCommandEvent*) {
              if (onRewind) {
                  onRewind();
              }
            });
        center.skipBackwardCommand.preferredIntervals = @[ @(10.0) ];

        // ==================================================

        // 系统媒体键：拖动进度（控制中心 / 触摸板的进度条）
        mImpl->seekTarget =
            makeCommandTarget(center.changePlaybackPositionCommand, ^(MPRemoteCommandEvent* event) {
              if ([event isKindOfClass:[MPChangePlaybackPositionCommandEvent class]] && onSeek) {
                  MPChangePlaybackPositionCommandEvent* seekEvent =
                      (MPChangePlaybackPositionCommandEvent*)event;
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
        // center.stopCommand.enabled = YES;
        center.skipForwardCommand.enabled = YES;
        center.skipBackwardCommand.enabled = YES;
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

        // 1. 将 std::string 转换为 NSString
        NSString* title = [NSString stringWithUTF8String:metadata.title.c_str()];
        nowPlaying[MPMediaItemPropertyTitle] = title;

        // 2. 先算出拼接后的 std::string，再转换为 NSString
        std::string artistsStr = SystemAudioControl::joinIntoString(metadata.artists, " / ");
        NSString* artists = [NSString stringWithUTF8String:artistsStr.c_str()];
        nowPlaying[MPMediaItemPropertyArtist] = artists;

        // FIXME(连接): 封面图尚未接入——MediaMetadata 结构体目前没有 cover 字段，
        //             若要显示封面请扩展 MediaMetadata 增加封面数据/路径，再填充
        //             MPMediaItemPropertyArtwork。

        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:nowPlaying];
    }
}

void MacosSystemAudioControl::updatePlaybackState(PlaybackState state) {
    if (!mImpl->initialized.load()) {
        return;
    }

    @autoreleasepool {
        NSMutableDictionary* info =
            [[[MPNowPlayingInfoCenter defaultCenter] nowPlayingInfo] mutableCopy];
        if (!info) {
            info = [NSMutableDictionary dictionary];
        }

        info[MPNowPlayingInfoPropertyPlaybackRate] =
            (state == PlaybackState::Play) ? @(1.0) : @(0.0);
        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:info];
    }
}

void MacosSystemAudioControl::shutdown() {
    if (!mImpl->initialized.exchange(false)) {
        return;
    }

    @autoreleasepool {
        MPRemoteCommandCenter* center = [MPRemoteCommandCenter sharedCommandCenter];

        if (mImpl->playTarget) {
            [center.playCommand removeTarget:mImpl->playTarget];
        }
        if (mImpl->pauseTarget) {
            [center.pauseCommand removeTarget:mImpl->pauseTarget];
        }
        if (mImpl->toggleTarget) {
            [center.togglePlayPauseCommand removeTarget:mImpl->toggleTarget];
        }
        if (mImpl->nextTarget) {
            [center.nextTrackCommand removeTarget:mImpl->nextTarget];
        }
        if (mImpl->previousTarget) {
            [center.previousTrackCommand removeTarget:mImpl->previousTarget];
        }
        if (mImpl->seekTarget) {
            [center.changePlaybackPositionCommand removeTarget:mImpl->seekTarget];
        }
        // if (mImpl->stopTarget) {
        //     [center.stopCommand removeTarget:mImpl->stopTarget];
        // }
        if (mImpl->fastForwardTarget) {
            [center.skipForwardCommand removeTarget:mImpl->fastForwardTarget];
        }
        if (mImpl->rewindTarget) {
            [center.skipBackwardCommand removeTarget:mImpl->rewindTarget];
        }

        [[MPNowPlayingInfoCenter defaultCenter] setNowPlayingInfo:nil];
    }

    mImpl->playTarget = nullptr;
    mImpl->pauseTarget = nullptr;
    mImpl->toggleTarget = nullptr;
    mImpl->nextTarget = nullptr;
    mImpl->previousTarget = nullptr;
    mImpl->seekTarget = nullptr;
    mImpl->fastForwardTarget = nullptr;
    mImpl->rewindTarget = nullptr;
}
