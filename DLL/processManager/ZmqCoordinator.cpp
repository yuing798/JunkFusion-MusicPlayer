#include "./AudioDefs.hpp"
#include "constants.h"
#include "otherUtils.hpp"
#include "spdlog/spdlog.h"
#include "zmq_coordinator.h"

bool ZmqCoordinator::start(const juce::File& exeFile, int timeoutMs) {
    auto logger{spdlog::get(LogDllID)};

    // ── 1. 绑定 REP 套接字，端口号用 0 让 zmq 自动分配 ──
    //    bind 到 "tcp://127.0.0.1:*" 后，通过 last_endpoint 拿回真实地址。
    // 创建了一个 REP（Reply，应答端） 类型的 Socket。
    // 角色定位：在请求-应答模式（Request-Reply）中，REP 是服务端。它的特点就是“收到请求 -> 处理 ->
    // 必须回复”。

    // DEALER（异步经销商） 🏆
    //  DEALER 是为异步双向通信量身打造的套接字类型。它的核心特性完美契合你的需求：

    // 完全异步（全双工自由）：和 PAIR 一样，send 和 recv 可以任意穿插，没有强制顺序。

    // 支持自动重连（关键优势）：如果子进程崩溃重启，新的 DEALER 连接可以无缝重新接入主进程的
    // DEALER，无需重启主进程。

    // 消息公平队列：ZeroMQ 会自动公平地轮询处理多个连接（虽然你这里只有 1
    // 个子进程，但扩展性更好）。
    mSocket = zmq::socket_t(mContext, zmq::socket_type::dealer);

    // 把这个 Socket 绑定到本机的 TCP 协议上。
    // 重点在于*（星号）：这个星号在这里代表
    // 端口号 0。在操作系统层面，端口 0
    // 是一个特殊的保留值，它的含义是：“我不在乎具体是哪个端口，请操作系统（OS内核）随便给我分配一个当前空闲的端口号”。
    // 127.0.0.1：只允许本机访问（绝对的“闭关锁国”）。
    // 0.0.0.0：监听本机所有网卡（包括 WiFi、以太网、虚拟网卡）
    // 192.168.1.100（局域网 IP）：同一个 WiFi/交换机下的其他同事或设备
    mSocket.bind("tcp://127.0.0.1:*"); // "*" = 端口 0，系统自动挑一个空闲端口

    // 获得随机分配的端口号
    std::string endpoint = mSocket.get(zmq::sockopt::last_endpoint); // 例如 "tcp://127.0.0.1:55555"

    // ── 2. 构造子进程命令行，把 zmq 地址塞进 --zmq-endpoint ──
    juce::StringArray args;
    args.add(exeFile.getFullPathName());

    args.add(AudioDefs::zmqEndpoint);
    args.add(juce::String(endpoint));
    args.add(AudioDefs::cacheDir);
    args.add(exeFile.getFullPathName());

    if (!mChildProcess.start(args)) {
        if (logger)
            logger->critical("启动后端进程失败: {}", exeFile.getFullPathName().toStdString());
        mSocket.close();
        return false;
    }
    if (logger) logger->info("后端进程已启动: {}", exeFile.getFullPathName().toStdString());

    // ── 3. 等待子进程发来 "ready"，收到后回发 "helloworld" ──
    mSocket.set(zmq::sockopt::rcvtimeo, timeoutMs); // 接收超时，防止子进程没连上时永久阻塞

    // zmq::message_t？
    //  支持二进制数据：std::string 以 \0 作为终止符，无法处理包含 \0
    //  的二进制数据（如图片、音频、序列化对象）。而 zmq::message_t
    //  可以存储任意字节，是真正的二进制安全。 零拷贝（Zero-Copy）传输：这是 ZeroMQ
    //  高性能的关键。发送 zmq::message_t
    //  时，数据可以被直接传递到网络缓冲区，避免了不必要的内存复制，极大地提升了吞吐量并降低了延迟。
    //  多部分消息（Multipart Messages）：ZeroMQ
    //  支持将多条消息组合成一个逻辑单元发送。zmq::message_t 是多部分消息的基本单元，而 std::string
    //  无法实现此功能。 与 ZeroMQ 内部机制集成：zmq::message_t 携带 ZeroMQ
    //  所需的元数据（如路由信息），这些对于 std::string 来说是不存在的。
    zmq::message_t request;

    // zmq::recv_flags::none 的意思是：“阻塞模式（Blocking Mode）”。
    //  它告诉 recv 函数：“在没有收到消息或者未超时之前，这个线程就死等在这里，绝对不返回。
    auto recvResult = mSocket.recv(request, zmq::recv_flags::none);

    if (!recvResult) { // 超时没收到 ready
        if (logger) logger->critical("等待子进程 ready 超时");
        stop();
        return false;
    }

    std::string ready(request.to_string());
    if (logger) logger->info("收到子进程握手信号: {}", ready);

    // 回发一句 helloworld
    zmq::message_t reply(std::string("helloworld"));
    mSocket.send(reply, zmq::send_flags::none);
    if (logger) logger->info("已回发 helloworld 给子进程");

    mRunning = true;
    return true;
}

void ZmqCoordinator::stop() {
    mChildProcess.kill();
    mSocket.close();
    mRunning = false;
}
