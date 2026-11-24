#include "AudioProcessCoordinator.h"
#include "../dllManager.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "spdlog/spdlog.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <zmq.hpp>
AudioProcessCoordinator::AudioProcessCoordinator() : juce::Thread("AudioProcessSupervisor") {
    mAudioProcessPusher = std::make_unique<AudioProcessPusher>(mContext, pushInitOK);
    mAudioProcessSuber = std::make_unique<AudioProcessSuber>(mContext, subInitOK);

    // 心跳回调必须轻：只写一个原子变量，别在 SUB 线程上做别的事
    mAudioProcessSuber->onHeartbeat = [this]() { noteHeartbeat(); };
}

bool AudioProcessCoordinator::start(const juce::File& exeFile, int oscPort, juce::File cacheDir) {
    auto logger{spdlog::get(LogDllID)};

    juce::StringArray args;
    args.add(exeFile.getFullPathName());

    // 等待端口分配完成
    pushInitOK.wait();
    logger->debug("成功等到push端口号{}", mAudioProcessPusher->getPort());
    args.add(AudioMacro::pushPullPort);
    args.add(juce::String(mAudioProcessPusher->getPort()));

    subInitOK.wait();
    logger->debug("成功等到sub端口号{}", mAudioProcessSuber->getPort());
    args.add(AudioMacro::pubSubPort);
    args.add(juce::String(mAudioProcessSuber->getPort()));
    args.add(AudioMacro::cacheDir);
    args.add(cacheDir.getFullPathName());
    args.add(AudioMacro::oscPort);
    args.add(juce::String(oscPort));

    // 命令行留着给看门狗重启时复用：端口是 Coordinator 持有的 socket，
    // 重启后不用变，新进程直接连回同样的端口
    mArgs = args;

    if (!spawnChild()) {
        if (logger)
            logger->critical("启动后端进程失败: {}", exeFile.getFullPathName().toStdString());
        return false;
    }
    if (logger) logger->info("后端进程已启动: {}", exeFile.getFullPathName().toStdString());

    mRunning = true;
    startThread(); // 看门狗
    return true;
}

bool AudioProcessCoordinator::spawnChild() {
    if (mArgs.isEmpty()) return false;

    // 心跳从"刚拉起来"开始计时：给新进程一个完整的超时窗口做初始化，
    // 否则第一轮轮询就会判它挂死
    mLastHeartbeatMs.store(juce::Time::currentTimeMillis());

    mChildProcess = std::make_unique<juce::ChildProcess>();
    return mChildProcess->start(mArgs);
}

void AudioProcessCoordinator::noteHeartbeat() noexcept {
    mLastHeartbeatMs.store(juce::Time::currentTimeMillis());
}

void AudioProcessCoordinator::run() {
    while (!threadShouldExit()) {
        juce::Thread::sleep(kPollIntervalMs);
        if (threadShouldExit() || !mRunning) continue;

        if (mChildProcess == nullptr || !mChildProcess->isRunning()) {
            handleChildLost("音频进程已退出");
            continue;
        }

        const auto last = mLastHeartbeatMs.load();
        if (last > 0 && juce::Time::currentTimeMillis() - last > kHeartbeatTimeoutMs) {
            handleChildLost("音频进程心跳超时（疑似挂死）");
        }
    }
}

void AudioProcessCoordinator::handleChildLost(const char* reason) {
    auto logger{spdlog::get(LogDllID)};
    if (logger) logger->critical("[Supervisor] {}，准备重启", reason);

    // 挂死的进程 kill 掉才能拿到干净的现场；正常退出的这里是个空操作
    if (mChildProcess != nullptr && mChildProcess->isRunning()) {
        mChildProcess->kill();
        juce::Thread::sleep(200);
    }
    mChildProcess.reset();

    const auto now = juce::Time::currentTimeMillis();
    if (mRestartWindowStart == 0 || now - mRestartWindowStart > kRestartWindowMs) {
        mRestartWindowStart = now;
        mRestartCount = 0;
    }
    ++mRestartCount;

    if (mRestartCount > kMaxRestartsInWindow) {
        // 一加载就崩的插件会把这里打满。设上限是为了防「崩溃→重启→再崩」的
        // 死循环；上层收到 -1 必须停下来让用户处理，不要自己再拉起。
        mRunning = false;
        if (logger)
            logger->critical(
                "[Supervisor] {} 秒内已重启 {} 次，放弃自动重启",
                static_cast<int>(kRestartWindowMs / 1000),
                kMaxRestartsInWindow
            );
        if (onAudioProcessRestarted) onAudioProcessRestarted(-1);
        return;
    }

    if (!spawnChild()) {
        // 拉不起来也走"丢了"这条路：下一轮轮询会再试，次数用完就放弃
        if (logger) logger->error("[Supervisor] 重启失败，下一轮重试");
        return;
    }

    if (logger) logger->info("[Supervisor] 已重启（第 {} 次）", mRestartCount);
    if (onAudioProcessRestarted) onAudioProcessRestarted(mRestartCount);
}

void AudioProcessCoordinator::stop() {
    // 先停看门狗：否则下面杀进程时它会立刻重新拉一个
    mRunning = false;
    if (isThreadRunning()) {
        signalThreadShouldExit();
        stopThread(2000);
    }

    if (mArgs.isEmpty() || mChildProcess == nullptr) return;

    Yvar obj = Yvar::object();
    obj.set(AudioMacro::killAudioProcess, "");
    mAudioProcessPusher->sendMessage(obj.dump());

    for (int time = 0; time < 300; time++) {
        if (!mChildProcess->isRunning()) return;

        if (time == 299) {
            mChildProcess->kill();
        } // 3秒没反应直接强杀
        juce::Thread::sleep(10);
    }
}

AudioProcessCoordinator::~AudioProcessCoordinator() {
    stop();
    juce::Thread::sleep(100);
    if (mAudioProcessPusher) {
        mAudioProcessPusher->stopThread(2000);
    }
    if (mAudioProcessSuber) {
        mAudioProcessSuber->stopThread(2000);
    }
}


AudioProcessPusher::AudioProcessPusher(zmq::context_t& context, juce::WaitableEvent& e)
    : juce::Thread("AudioProcessPusher"), mContext(context), portInitOK(e) {
    startThread();
}

void AudioProcessPusher::sendMessage(const std::string& msg) {
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(msg);
    wakeUpEvent.signal(); // 唤醒沉睡的发送线程
}

void AudioProcessPusher::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::push);
    socket.bind("tcp://127.0.0.1:*"); // 主进程bind,子进程connect
    mPushPullPort = socket.get(zmq::sockopt::last_endpoint);
    portInitOK.signal(); // 跨线程供给必须需要waitableEvent,防止收不到
    spdlog::get(LogDllID)->debug("已获取协调者：push端口为{}", mPushPullPort);

    juce::Thread::sleep(100);

    while (!threadShouldExit()) {
        // 线程睡眠在此，等待被 sendMessage 唤醒。超时设为 500ms 方便退出检查
        wakeUpEvent.wait(500);

        std::queue<std::string> localQueue;
        {
            // td::lock_guard<std::mutex> 是 C++ 标准库提供的一个 RAII（资源获取即初始化）
            // 锁管理器。简单来说，它是 “自动锁”
            // std::lock_guard<Mutex>（锁守卫）：这是一个类模板。它的构造函数会调用
            // mutex.lock()，它的析构函数会调用 mutex.unlock()。
            std::lock_guard<std::mutex> lock(queueMutex);
            std::swap(localQueue, messageQueue); // 快速把队列交换出来，减少锁占用时间
        }

        while (!localQueue.empty()) {
            std::string msg = localQueue.front();
            localQueue.pop();

            zmq::message_t zmsg(msg.data(), msg.size());
            socket.send(zmsg, zmq::send_flags::none);
        }
    }
    socket.close();
}

AudioProcessPusher::~AudioProcessPusher() {
    // stopThread(2000);
}

AudioProcessSuber::AudioProcessSuber(zmq::context_t& context, juce::WaitableEvent& e)
    : juce::Thread("AudioProcessSuber"), mContext(context), portInitOK(e) {
    startThread();
}

void AudioProcessSuber::run() {
    zmq::socket_t socket(mContext, zmq::socket_type::sub);
    socket.bind("tcp://127.0.0.1:*");
    mPubSubPort = socket.get(zmq::sockopt::last_endpoint);
    portInitOK.signal();
    spdlog::get(LogDllID)->debug("已获取协调者：sub端口为{}", mPubSubPort);

    // 设置订阅过滤器
    socket.set(zmq::sockopt::subscribe, ""); // 默认接收所有消息
    socket.set(zmq::sockopt::rcvtimeo, 500); // 超时时间为500ms
    while (!threadShouldExit()) {

        zmq::message_t msg;
        auto rst = socket.recv(msg, zmq::recv_flags::none); // 同步阻塞等待

        if (rst.has_value()) {
            // Utils::writeEmergencyLog("suber收到消息");
            std::string data(static_cast<char*>(msg.data()), msg.size());
            Yvar obj = Yvar::parse(data);
            // spdlog::get(LogDllID)->debug("suber收到消息:{}", obj.toString());
            if (!obj.isObject()) {
                spdlog::get(LogDllID)->error("AudioProcessSuber接收到未知格式:{}", obj.toString());
                continue;
            };
            // 心跳：最先处理，因为它每 500ms 就来一条，必须最省
            if (obj.has(AudioMacro::heartbeat)) {
                if (onHeartbeat) onHeartbeat();
                continue;
            }
            // 全局效果器链的状态回报（每个槽加载成功没有 / 延迟）
            if (obj.has(AudioMacro::pluginChainState)) {
                if (auto* chainController = dllManager::getInstance().getPluginChainController()) {
                    chainController->onStateFromAudio(
                        obj.read(AudioMacro::pluginChainState)
                    );
                }
                continue;
            }
            if (obj.has(AudioMacro::errorPopupWindowMsg)) {
                auto receiverMsg = obj.read(AudioMacro::errorPopupWindowMsg).toString();
                spdlog::get(LogDllID)->debug("sub接收到发送错误弹窗消息:{}", receiverMsg);
                // Utils::writeEmergencyLog("sub接收到发送错误弹窗消息");
                auto cString{ConvertUtils::copyStringOnHeap(receiverMsg)};

                if (dllManager::getInstance().onErrorSendCallback)
                    dllManager::getInstance().onErrorSendCallback(cString);
                continue;
            }
            if (obj.has(AudioMacro::fatalErrorPopupWindowMsg)) {
                auto receiverMsg = obj.read(AudioMacro::fatalErrorPopupWindowMsg).toString();
                spdlog::get(LogDllID)->error("sub接收到发送致命错误弹窗消息:{}", receiverMsg);
                auto cString{ConvertUtils::copyStringOnHeap(receiverMsg)};

                if (dllManager::getInstance().onFatalErrorSendCallback)
                    dllManager::getInstance().onFatalErrorSendCallback(cString);
                continue;
            }
            if (obj.has(AudioMacro::currentPTS)) {

                double pts = obj.read(AudioMacro::currentPTS).toDouble();
                // spdlog::get(LogDllID)->debug("suber接收到信息:准备发送当前的pts回调:{}", pts);
                if (dllManager::getInstance().onCurrentPTSCallback)
                    dllManager::getInstance().onCurrentPTSCallback(pts);
                continue;
            }
            if (obj.has(AudioMacro::onPlayNextOrPreviousSong)) {
                if (dllManager::getInstance().onPlayNextOrPreviousSong) {
                    auto state{obj.read(AudioMacro::onPlayNextOrPreviousSong).toBool()};
                    dllManager::getInstance().onPlayNextOrPreviousSong(state);
                }

                continue;
            }
            if (obj.has(AudioMacro::updatePlayCount)) {
                auto path{obj.read(AudioMacro::updatePlayCount).toString()};
                dllManager::getInstance().getDbContext()->songsManage.updatePlayCount(path);
            }
            if (obj.has(AudioMacro::playStateSync)) {
                if (dllManager::getInstance().onPlayStateSync) {
                    dllManager::getInstance().onPlayStateSync(
                        obj.read(AudioMacro::playStateSync).toInt()
                    );
                }
            }
        }
    }

    socket.close();
}

AudioProcessSuber::~AudioProcessSuber() {
    // stopThread(2000);
}