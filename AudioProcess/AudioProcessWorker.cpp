#include "./AudioProcessWorker.hpp"
#include "./AudioDefs.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>

AudioProcessWorker::AudioProcessWorker()
    : juce::Thread("AudioProcessListener"), mGodProcessor(mSocket) {}

bool AudioProcessWorker::initialise(const std::string& endpoint) {

    mSocket = zmq::socket_t(mContext, zmq::socket_type::dealer);
    mSocket.set(zmq::sockopt::rcvtimeo, 5000);
    mSocket.set(zmq::sockopt::sndtimeo, 5000);
    try {
        mSocket.connect(endpoint);
    } catch (const zmq::error_t& e) {
        // auto log{spdlog::get(LogAudioID)};
        // log->error("[AudioProcess]工作者链接")
        Utils::writeEmergencyLog((std::string("【AudioProcess】链接失败") + e.what()).c_str());
        return false;
    }
    startThread(); // 启动run函数

    return true;
}
void AudioProcessWorker::run() {
    while (!threadShouldExit()) {
        zmq::message_t request;

        try {
            auto res = mSocket.recv(request, zmq::recv_flags::none);

            // 如果收到数据（没有超时）
            if (res.has_value()) {
                // 转为字符串
                std::string msgStr(static_cast<char*>(request.data()), request.size());

                auto obj{juce::JSON::parse(juce::String{msgStr})};
                if (obj.getDynamicObject()->hasProperty(AudioDefs::songPath)) {
                    auto songPath =
                        obj.getDynamicObject()->getProperty(AudioDefs::songPath).toString();
                }
            }
        } catch (const zmq::error_t& e) {
            auto log{spdlog::get(LogAudioID)};
            log->error("zmq监听工作者发生异常:{}", e.what());
        }
    }
}

AudioProcessWorker::~AudioProcessWorker() { stopThread(1500); }
