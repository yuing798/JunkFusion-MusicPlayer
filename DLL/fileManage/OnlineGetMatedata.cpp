#include "./OnlineGetMatedata.hpp"
#include "../dllUtils.hpp"
#include "constants.h"
#include "dllManager.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <SQLiteCpp/Database.h>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <mutex>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

OnlineGetMatedata::OnlineGetMatedata(SQLite::Database& db)
    : juce::Thread("OnlineGetMatedata"), db(db) {}

void OnlineGetMatedata::setTask(OnlineGetMatedata::Task task) {
    std::lock_guard<std::mutex> lock{mtx};
    mTaskQueue.push(std::move(task));
    notify();
    if (!isThreadRunning()) startThread();
}
void OnlineGetMatedata::run() {
    while (!threadShouldExit()) {
        Task task;
        bool hasTask = false;

        // 1. 缩小锁的作用域，仅在弹出任务时持锁
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!mTaskQueue.empty()) {
                task = std::move(mTaskQueue.front());
                mTaskQueue.pop();
                hasTask = true;
            }
        } // 锁在此处自动释放

        // 2. 根据是否有任务决定处理还是休眠
        if (hasTask) {
            searchDataByPrint(std::move(task)); // 在锁外执行耗时任务
        } else {
            // 在【无锁状态】下安全挂起等待新任务！
            wait(-1);
        }
    }
}

void OnlineGetMatedata::searchDataByPrint(OnlineGetMatedata::Task task) {
    spdlog::get(LogDllID)->debug("开始根据指纹搜索元数据");
    std::string client_key = "lp1ajcs0Bd";

    // 发起 POST 请求
    cpr::Response res = cpr::Post(
        cpr::Url{"https://api.acoustid.org/v2/lookup"},
        // cpr::Payload 会自动将参数放在 HTTP Body 中以 POST 表单发出
        cpr::Payload{
            {"client", client_key},
            {"meta", "recordings + releases + tracks"}, // 一次性拿全元数据
            {"duration", std::to_string((int)task.duration)},
            {"fingerprint", task.print},
            {"format", "json"}
        },
        cpr::Timeout{5000} // 5秒超时
    );

    if (res.status_code == 200) {
        // std::cout << "请求成功！返回 JSON 数据：" << std::endl;
        spdlog::get(LogDllID)->debug("得到网络请求返回的表单数据:{}", res.text);

        juce::var v{juce::JSON::fromString(juce::String(res.text))};
        auto recording{v["results"][0]["recordings"][0]};
        auto recordingId{
            recording["id"].toString().toStdString()
        }; // 这个是歌曲的ID，用来搜索艺术家名称
        auto release{recording["releases"][0]};
        auto releaseId{release["id"].toString().toStdString()}; // 这个是专辑的ID，用来搜索专辑图片
        if (task.needTitle) {
            auto title{release["mediums"][0]["tracks"][0]["title"]};
        }

        if (task.needArtist) {
            // 因为多位艺术家需要直接去MB官方服务器才能查到
            cpr::Response artistsRes = cpr::Get(
                cpr::Url{"https://musicbrainz.org/ws/2/recording/" + recordingId},
                cpr::Parameters{
                    {"inc", "artists"}, // 核心参数：要求展开详细的艺术家数组
                    {"fmt", "json"}
                },
                // 强制要求：MusicBrainz 必须携带包含联系方式的 User-Agent，否则直接返回403 错误
                cpr::Header{{"User-Agent", "JunkFusion/1.0.0 ( yusekx@gmail.com )"}},
                cpr::Timeout{5000}
            );
            if (artistsRes.status_code == 200) {
                spdlog::get(LogDllID)->debug("搜索艺术家名称返回:{}", artistsRes.text);
            } else {
                spdlog::get(LogDllID)->debug(
                    "搜索艺术家失败:错误代码{}:错误内容:{}",
                    artistsRes.status_code,
                    artistsRes.text
                );
            }
            juce::Thread::sleep(1000); // 妈的MB服务器要求必须睡一秒才能再次发送
        }
        if (task.needAlbum) {
            auto album{release["title"].toString().toStdString()};
        }
        if (task.needCover) {
            cpr::Response coverRes = cpr::Get(
                cpr::Url{"http://coverartarchive.org/release/" + releaseId + "/front"},
                cpr::Header{{"User-Agent", "JunkFusion/1.0.0 ( yusekx@gmail.com )"}},
                cpr::Timeout{10000} // 图片下载可能较慢，建议把超时设长一点
            );

            if (res.status_code == 200) {
                do {
                    SHA1 sha1;
                    auto hash = sha1(coverRes.text.data(), coverRes.text.size());

                    juce::File hashImageDir{
                        dllManager::getInstance().getSongImageDir().getChildFile(hash)
                    };
                    // data存在说明一定有图片，所以直接使用.value()就行了
                    //  直接用哈希值作为文件夹名，所有该图片相关的缓存文件都放在同一个文件夹中

                    if (hashImageDir.exists()) {
                        break;
                    } else {
                        hashImageDir.createDirectory();
                    } // 如果这个目录已经存在，直接退出，避免保存两个相同图片

                    juce::File originalFile{hashImageDir.getChildFile("original.jpg")};

                    juce::FileOutputStream outputStream(originalFile);
                    if (outputStream.openedOk()) {
                        outputStream.write(coverRes.text.data(), coverRes.text.size());
                        outputStream.flush();
                    }

                } while (0);
            } else {
                spdlog::get(LogDllID)->debug(
                    "搜索专辑图片失败:错误代码{}:错误内容:{}",
                    coverRes.status_code,
                    coverRes.text
                );
            }
        }

    } else {
        spdlog::get(LogDllID)
            ->error("请求失败！错误码: {} | 服务器返回的错误信息: {}", res.status_code, res.text);
    }
}