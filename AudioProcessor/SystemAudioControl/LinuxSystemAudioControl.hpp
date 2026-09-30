#pragma once

#include "SystemAudioControl.hpp"

#include <memory>

class LinuxSystemAudioControl : public SystemAudioControl {
public:
    LinuxSystemAudioControl();
    ~LinuxSystemAudioControl() override;

    bool initialize() override;
    void updateMetadata(const MediaMetadata& metadata) override;
    void updatePlaybackState(PlaybackState state) override;
    void shutdown() override;

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};
