#pragma once

// zmq_coordinator.h —— 主进程(DLL)侧的后端进程协调者
//
// 职责：
//  1. 绑定一个 zmq REP 套接字(监听一个临时端口)，供子进程连接
//  2. 通过 juce::ChildProcess 启动 "JunkFusionAudioProcess.exe"，并把 zmq 地址
//     通过命令行参数 --zmq-endpoint=<地址> 传给子进程
//  3. 等待子进程用 REQ 套接字连上来并发送 "ready"（即"子进程初始化完成"）
//  4. 收到 ready 后，向子进程回发一句 "helloworld"，完成握手
//
// 采用 REP/REQ 模式而不是 PUSH/PULL 的原因：
//  REP(协调者)/REQ(工作者) 天然就是"一问一答"，正好满足"子进程先初始化，
//  主进程再发消息"的顺序要求；而且 REP 套接字会把 REQ 发来的帧存住，
//  不会像 PUSH 那样在 PULL 还没连上时把消息悄悄丢掉(fast-joiner 问题)。

#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <memory>
#include <string>

class ZmqCoordinator {
public:
    // 启动子进程并完成握手。
    //   exeFile : 后端进程( JunkFusionAudioProcess.exe )的完整路径
    //   timeoutMs : 等待子进程连上来并发出 ready 的超时时间
    // 返回 true 表示子进程已启动且收到了它的 ready(握手成功)。失败返回 false。
    bool start(const juce::File& exeFile, int timeoutMs = 5000);

    // 关闭套接字，并强制结束子进程。
    void stop();

private:
    juce::ChildProcess mChildProcess; // 负责拉起并监控后端进程
    zmq::context_t mContext{1};       // zmq 上下文(单 IO 线程足够)
    zmq::socket_t mSocket;            // REP 套接字(协调者绑定的一端),实际收发消息的接口
    bool mRunning{false};
};
