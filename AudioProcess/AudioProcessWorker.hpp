#pragma once

#include "./DeviceManager.hpp"
#include "GodProcessor.hpp"
#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <memory>
#include <string>

class AudioProcessWorker : public juce::Thread {
public:
    AudioProcessWorker(juce::File cacheDir);
    bool initialise(const std::string& endpoint);
    void run() override;
    ~AudioProcessWorker();

private:
    zmq::context_t mContext{1}; // zmq 上下文
    zmq::socket_t mSocket;
    std::unique_ptr<DeviceManager> mManager;
    GodProcessor mGodProcessor;
    juce::File mCacheDir;
};
