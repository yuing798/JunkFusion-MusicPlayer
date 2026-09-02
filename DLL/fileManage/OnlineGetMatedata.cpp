#include "./OnlineGetMatedata.hpp"
#include "../dllUtils.hpp"
#include "constants.h"
#include "dllManager.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <queue>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

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
    spdlog::get(LogDllID)->debug("开始根据指纹搜索元数据:歌曲ID:{}", task.songId);

    task.album = std::nullopt;
    task.artist = std::nullopt;
    task.needCover = true;
    task.title = std::nullopt; // 测试
    std::string title, artist, album;

    // 发起 POST 请求
    cpr::Response res = cpr::Post(
        cpr::Url{"https://api.acoustid.org/v2/lookup"},
        // cpr::Payload 会自动将参数放在 HTTP Body 中以 POST 表单发出
        cpr::Payload{
            {"client", client_key},
            {"meta", "recordings + releases"},
            {"duration", std::to_string((int)task.duration)},
            {"fingerprint", task.print},
            {"format", "json"}
        },
        cpr::Timeout{5000} // 5秒超时
    );

    if (res.status_code == 200) {
        // std::cout << "请求成功！返回 JSON 数据：" << std::endl;
        spdlog::get(LogDllID)->debug("得到网络请求返回的表单数据:{}", res.text);

        struct SearchScore {
            double score{100.0}; // 最终匹配度得分
            std::string recordingId;
        };
        std::vector<SearchScore> searchScores;

        Utils::Yvar v{juce::JSON::fromString(juce::String(res.text))};
        auto results{v.read("results")};

        for (int resultIndex = 0; resultIndex < results.size(); resultIndex++) {
            double printScore{results.read(resultIndex).read("score").toDouble()};
            // 原始分数
            if (printScore <= 0.7) {
                continue;
            } // 匹配度小于0.7直接退出

            for (int recordingIndex = 0;
                 recordingIndex < results.read(resultIndex).read("recordings").size();
                 recordingIndex++) {

                SearchScore searchScore;
                searchScore.score *= printScore;
                auto recording{results.read(resultIndex).read("recordings").read(recordingIndex)};
                auto recordingDuration{recording.read("duration").toDouble()};
                auto durationScore{1 - std::abs(recordingDuration - task.duration) / task.duration};
                if (durationScore <= 0.85) {
                    continue;
                } else {
                    searchScore.score *= durationScore;
                    searchScore.recordingId = recording.read("id").toString().toStdString();
                } // 根据歌曲时长匹配分数

                if (task.title != std::nullopt) {
                    // 有原本的标题
                    auto titleScore = Utils::stringSimilarity(
                        recording.read("title").toString(),
                        juce::String(task.title.value())
                    );
                    searchScore.score *= titleScore;
                }
            }
        }

        int resultCount{0};
        while (true) {
            if (resultCount >= results.size()) {
                spdlog::get(LogDllID)->info("查不到recordings");
                return;
            }
            if (results.read(resultCount).hasProperty("recordings")) break;

            resultCount++;
        }
        if (v.read("results").read(resultCount).toDouble() < 0.9) {
            spdlog::get(LogDllID)->debug("歌曲{}指纹匹配度过低，直接退出", task.songId);
            return;
        }
        auto recordings{v.read("results").read(resultCount).read("recordings")};

        auto recording{v.read("results").read(0).read("recordings").read(0)};

        // auto release{recording["releases"][0]};
        // auto releaseId{release["id"].toString().toStdString()}; //
        // 这个是专辑的ID，用来搜索专辑图片
        // if (task.needTitle) {
        //     title = recording.read("releases")
        //                 .read(0)
        //                 .read("mediums")
        //                 .read(0)
        //                 .read("tracks")
        //                 .read(0)
        //                 .read("title")
        //                 .toString()
        //                 .toStdString();
        //     spdlog::get(LogDllID)->debug("获取歌曲名:{}", title);
        // }

        // if (task.needArtist || task.needCover) {
        //     // 因为多位艺术家需要直接去MB官方服务器才能查到
        //     auto recordingId{
        //         recording.read("id").toString().toStdString()
        //     }; // 这个是歌曲的ID，用来搜索艺术家名称
        //     cpr::Response artistsRes = cpr::Get(
        //         cpr::Url{"https://musicbrainz.org/ws/2/recording/" + recordingId},
        //         cpr::Parameters{
        //             {"inc", "artists"}, // 核心参数：要求展开详细的艺术家数组
        //             {"fmt", "json"}
        //         },
        //         // 强制要求：MusicBrainz 必须携带包含联系方式的 User-Agent，否则直接返回403 错误
        //         cpr::Header{{"User-Agent", "JunkFusion/1.0.0 ( yusekx@gmail.com )"}},
        //         cpr::Timeout{5000}
        //     );
        //     if (artistsRes.status_code == 200) {
        //         spdlog::get(LogDllID)->debug("搜索艺术家名称返回:{}", artistsRes.text);
        //     } else {
        //         spdlog::get(LogDllID)->debug(
        //             "搜索艺术家失败:错误代码{}:错误内容:{}",
        //             artistsRes.status_code,
        //             artistsRes.text
        //         );
        //     }
        //     juce::Thread::sleep(1000); // 妈的MB服务器要求必须睡一秒才能再次发送
        // }
        // if (task.needAlbum) {
        //     album = recording.read("releases").read(0).read("title").toString().toStdString();
        //     spdlog::get(LogDllID)->debug("获取专辑名称:{}", album);
        // }
        // if (task.needCover) {
        //     // auto releaseLen{recording["releases"].getArray()->size()};
        //     // spdlog::get(LogDllID)->debug("该歌曲一共有{}张release", releaseLen);
        //     for (int i = 0; i < releaseLen; i++) {
        //         // spdlog::get(LogDllID)->debug("开始搜索第{}张release的封面", i);
        //         // auto release{recording["releases"][i]}; // 多搜索几张
        //         // auto releaseId{
        //         //     release["id"].toString().toStdString()
        //         // }; // 这个是专辑的ID，用来搜索专辑图片
        //         // cpr::Response coverRes = cpr::Get(
        //         //     cpr::Url{"https://coverartarchive.org/release/" + releaseId + "/front"},
        //         //     cpr::Header{{"User-Agent", "JunkFusion/1.0.0 ( yusekx@gmail.com )"}},
        //         //     cpr::Redirect{
        //         //         10L // maximum redirects
        //         //     },
        //         //     cpr::Timeout{10000} // 图片下载可能较慢，建议把超时设长一点
        //         // );

        //         std::string searchTerm = artistName + " " + albumName;

        //         spdlog::get(LogDllID)->debug("开始在 iTunes 搜索封面: {}", searchTerm);

        //         // 2. 发起 iTunes 搜索 API 请求
        //         cpr::Response coverRes = cpr::Get(
        //             cpr::Url{"https://itunes.apple.com/search"},
        //             cpr::Parameters{
        //                 {"term", searchTerm},
        //                 {"media", "music"},
        //                 {"entity", "album"}, // entity=album 代表搜索专辑，如果是单曲可以改为
        //                 "song"
        //                 {"limit", "1"}       // 只要最匹配的第 1 条结果
        //             },
        //             cpr::Timeout{10000}
        //         );

        //         if (coverRes.status_code == 200) {
        //             do {
        //                 SHA1 sha1;
        //                 auto hash = sha1(coverRes.text.data(), coverRes.text.size());

        //                 juce::File hashImageDir{
        //                     dllManager::getInstance().getSongImageDir().getChildFile(hash)
        //                 };
        //                 // data存在说明一定有图片，所以直接使用.value()就行了
        //                 // 直接用哈希值作为文件夹名，所有该图片相关的缓存文件都放在同一个文件夹中

        //                 if (hashImageDir.exists()) {
        //                     break;
        //                 } else {
        //                     hashImageDir.createDirectory();
        //                 } // 如果这个目录已经存在，直接退出，避免保存两个相同图片

        //                 juce::File originalFile{hashImageDir.getChildFile("original.jpg")};
        //                 spdlog::get(LogDllID)->debug("成功获取封面:哈希值:{}", hash);

        //                 juce::FileOutputStream outputStream(originalFile);
        //                 if (outputStream.openedOk()) {
        //                     outputStream.write(coverRes.text.data(), coverRes.text.size());
        //                     outputStream.flush();
        //                 }

        //             } while (0);

        //             break;
        //         } else {
        //             spdlog::get(LogDllID)->debug(
        //                 "搜索专辑图片失败:错误代码{}:错误内容:{}:底层错误:{}",
        //                 coverRes.status_code,
        //                 coverRes.text,
        //                 coverRes.error.message
        //             );
        //         }
        //     }
        // }

    } else {
        spdlog::get(LogDllID)
            ->error("请求失败！错误码: {} | 服务器返回的错误信息: {}", res.status_code, res.text);
    }
    spdlog::get(LogDllID)->debug("联网搜索歌曲元数据完成，歌曲ID:{}", task.songId);
}