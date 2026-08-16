#pragma once

#include "./DeviceManager.hpp"
#include "GodProcessor.hpp"
#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <memory>
#include <string>

class AudioProcessWorker : public juce::Thread {
public:
    AudioProcessWorker(juce::File cacheDir, std::string tcpPort);
    void run() override;
    ~AudioProcessWorker();

private:
    // zmq 上下文
    zmq::context_t mContext{1};
    zmq::socket_t mSocket;
    // std::unique_ptr<DeviceManager> mManager;
    // std::unique_ptr<GodProcessor> mGodProcessor;
    juce::File mCacheDir;
    std::string mTcpPort;
};
