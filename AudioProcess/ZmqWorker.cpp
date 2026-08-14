#include "constants.h"
#include "otherUtils.hpp"
#include "zmq_worker.h"
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>

bool ZmqWorker::initialise(const std::string& endpoint) {

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

    return true;
}
