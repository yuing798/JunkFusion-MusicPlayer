#pragma once
#include "juce_osc/juce_osc.h"
class OscSender {
private:
    juce::OSCSender mSender;
    int port;

public:
    OscSender();
    ~OscSender();
    int getPort() noexcept { return port; };
    void sendMsg(juce::OSCMessage& msg) { mSender.send(msg); }
};