#include "./OscSender.hpp"
#include "constants.h"
#include <spdlog/spdlog.h>

OscSender::OscSender() {

    // 1. 探路：创建一个临时的 UDP Socket
    juce::DatagramSocket tempSocket(false);

    // 2. 绑定到 0，强迫系统分配一个空闲端口
    tempSocket.bindToPort(0);

    // 3. 把这个宝贵的空闲端口号记下来（比如系统给了 54321）
    port = tempSocket.getBoundPort();

    // 4. 销毁临时 Socket，把这个端口释放出来（作用域结束自动销毁）
    tempSocket.shutdown();

    bool isConnected = mSender.connect("127.0.0.1", port);

    auto log{spdlog::get(LogDllID)};
    if (isConnected) {
        log->info("OSC Sender 初始化成功，连接至 127.0.0.1:{}", port);
    } else {
        // 在实际项目中，如果这里失败，通常是因为系统防火墙拦截
        log->error("警告：OSC Sender 连接失败！");
    }
}