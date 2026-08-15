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
    bool initWorker(const std::string& endpoint);
    void run() override;
    ~AudioProcessWorker();

private:
    zmq::context_t mContext{1}; // zmq 上下文
    zmq::socket_t mSocket; // 这个逼玩意是非线程安全的，所以不要跨线程使用
    // std::unique_ptr<DeviceManager> mManager;
    // std::unique_ptr<GodProcessor> mGodProcessor;
    juce::File mCacheDir;
};
