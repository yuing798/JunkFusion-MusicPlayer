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

AudioProcessWorker::AudioProcessWorker(juce::File cacheDir)
    : juce::Thread("AudioProcessListener"), mCacheDir(cacheDir) {
    juce::File configFile{mCacheDir.getChildFile("JFConfig.xml")};
    if (!configFile.existsAsFile()) configFile.create();
    // mManager = std::make_unique<DeviceManager>(mSocket, std::move(configFile));
    // mGodProcessor = std::make_unique<GodProcessor>(mSocket);
    // mManager->connectProcessor(mGodProcessor.get());
}

bool AudioProcessWorker::initWorker(const std::string& endpoint) {

    mSocket = zmq::socket_t(mContext, zmq::socket_type::dealer);
    mSocket.set(zmq::sockopt::rcvtimeo, 5000);
    mSocket.set(zmq::sockopt::sndtimeo, 5000);
    try {
        mSocket.connect(endpoint);
    } catch (const zmq::error_t& e) {
        Utils::writeEmergencyLog((std::string("【AudioProcess】链接失败") + e.what()).c_str());
        return false;
    }

    // 主动握手：DEALER 的 connect 只是向 zmq 后台线程登记一个"待建立"的连接，纯异步返回；
    // 不主动发消息，连接就不会被激活，协调者之后发的 kill 也送不到这里。
    // 这里立刻 send 一条 "ready"：zmq 会先把消息排队，等 TCP 连接真正建立后自动发出，
    // 从而同时完成"激活连接"和"告知协调者我已就绪"两件事。
    std::string handshake = "ready";
    zmq::message_t handshakeMsg(handshake.data(), handshake.size());
    auto sendResult = mSocket.send(handshakeMsg, zmq::send_flags::none);
    if (!sendResult.has_value()) {
        Utils::writeEmergencyLog("【AudioProcess】握手消息发送失败");
        return false;
    }
    Utils::writeEmergencyLog("【AudioProcess】已发送握手消息");

    startThread(); // 启动run函数

    return true;
}
void AudioProcessWorker::run() {
    Utils::writeEmergencyLog("开始调用工作者监听信息");
    while (!threadShouldExit()) {
        zmq::message_t request;
        Utils::writeEmergencyLog("监听中");

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
