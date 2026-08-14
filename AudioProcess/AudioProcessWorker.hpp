#pragma once

#include "GodProcessor.hpp"
#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <string>

class AudioProcessWorker : public juce::Thread {
public:
    AudioProcessWorker();
    bool initialise(const std::string& endpoint);
    void run() override;
    ~AudioProcessWorker();

private:
    zmq::context_t mContext{1}; // zmq 上下文
    zmq::socket_t mSocket;
    GodProcessor mGodProcessor;
};
