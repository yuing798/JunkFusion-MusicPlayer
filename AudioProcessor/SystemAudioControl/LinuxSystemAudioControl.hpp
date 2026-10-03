#pragma once

#include "SystemAudioControl.hpp"

#include <memory>

#ifdef __linux__
    #include <dbus/dbus.h>
#endif

class LinuxSystemAudioControl : public SystemAudioControl {
public:
    LinuxSystemAudioControl();
    ~LinuxSystemAudioControl() override;

    bool initialize() override;
    void updateMetadata(const MediaMetadata& metadata) override;
    void updatePlaybackState(PlaybackState state) override;
    void shutdown() override;

#ifdef __linux__
    static DBusHandlerResult
    handleMessage(DBusConnection* connection, DBusMessage* msg, void* userData);
#endif

private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};
