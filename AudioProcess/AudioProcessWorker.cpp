#include "./AudioProcessWorker.hpp"
#include "./AudioDefs.hpp"
#include "DeviceManager.hpp"
#include "GodProcessor.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>

AudioProcessWorker::AudioProcessWorker(juce::File cacheDir, std::string tcpPort)
    : juce::Thread("AudioProcessListener"), mCacheDir(cacheDir), mTcpPort(tcpPort) {
    juce::File configFile{mCacheDir.getChildFile("JFConfig.xml")};
    if (!configFile.existsAsFile()) configFile.create();
    // mManager = std::make_unique<DeviceManager>(mSocket, std::move(configFile));
    // mGodProcessor = std::make_unique<GodProcessor>(mSocket);
    // mManager->connectProcessor(mGodProcessor.get());
    startThread();
}

void AudioProcessWorker::run() {

    mSocket = zmq::socket_t(mContext, zmq::socket_type::dealer);
    mSocket.set(zmq::sockopt::rcvtimeo, 5000);
    mSocket.set(zmq::sockopt::sndtimeo, 5000);
    try {
        mSocket.connect(mTcpPort);
    } catch (const zmq::error_t& e) {
        auto log{spdlog::get(LogAudioID)};
        log->error("无法和协调者握手:{}", e.what());
        return;
    }
    // std::string handshake = "ready";
    // zmq::message_t handshakeMsg(handshake.data(), handshake.size());
    // auto sendResult = mSocket.send(handshakeMsg, zmq::send_flags::none);
    // if (!sendResult.has_value()) {
    //     Utils::writeEmergencyLog("【AudioProcess】握手消息发送失败");
    // }
    // Utils::writeEmergencyLog("【AudioProcess】已发送握手消息");

    // zmq::message_t ping;
    // auto recvResult = mSocket.recv(ping, zmq::recv_flags::none);
    // if (recvResult.has_value()) {
    //     std::string handshakeStr(static_cast<char*>(ping.data()), ping.size());
    //     Utils::writeEmergencyLog(("收到worker握手:" + handshakeStr).c_str());
    // } else {
    //     Utils::writeEmergencyLog("等待worker握手超时");
    // }

    Utils::writeEmergencyLog("开始调用工作者监听信息");
    while (!threadShouldExit()) {
        zmq::message_t request;

        auto res = mSocket.recv(request, zmq::recv_flags::none);

        // 如果收到数据（没有超时）
        if (res.has_value()) {
            // 转为字符串
            std::string msgStr(static_cast<char*>(request.data()), request.size());
            Utils::writeEmergencyLog("收到zmq数据");
            Utils::writeEmergencyLog(msgStr.c_str());

            if (msgStr == "kill") {
                Utils::writeEmergencyLog("接收到dll命令，准备kill音频进程");
                juce::MessageManager::callAsync([]() {
                    juce::MessageManager::getInstance()->stopDispatchLoop();
                });
                break;
            }

            auto obj{juce::JSON::parse(juce::String{msgStr})};
            if (obj.getDynamicObject()->hasProperty(AudioDefs::songPath)) {
                auto songPath = obj.getDynamicObject()->getProperty(AudioDefs::songPath).toString();
            }
        } else {
            Utils::writeEmergencyLog("receive链接超时");
        }
    }
}

AudioProcessWorker::~AudioProcessWorker() {
    stopThread(1500);
    // mManager->disconnectProcessor();
}
