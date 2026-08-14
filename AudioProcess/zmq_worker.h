#pragma once

// zmq_worker.h —— 后端进程(子进程)侧的工作者
//
// 职责：
//  1. 从命令行参数里解析出协调者传来的 zmq 地址(--zmq-endpoint=<地址>)
//  2. 用 REQ 套接字连接协调者的 REP 套接字
//  3. 连上后立刻发送一句 "ready"，告知协调者"我已经初始化完成"
//  4. 阻塞接收协调者回复的消息(当前的约定是第一句就是 "helloworld")
//
// 与协调者 ZmqCoordinator(REP) 组成一问一答的 REQ 端。

#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <string>

class ZmqWorker {
public:
    bool initialise(const std::string& endpoint);

private:
    zmq::context_t mContext{1}; // zmq 上下文
    zmq::socket_t mSocket;      // REQ 套接字(工作者连接的一端)
};
