#include "./OnlineGetMatedata.hpp"
#include "../dllUtils.hpp"
#include "constants.h"
#include "dllManager.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <algorithm>
#include <cpr/api.h>
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

bool OnlineGetMatedata::isVariousArtists(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return (
        lower.find("various") != std::string::npos || lower.find("群星") != std::string::npos ||
        lower == "v.a." || lower == "va"
    );
}

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
    task.artists = std::vector<std::string>{};
    task.needCover = true;
    task.title = std::nullopt; // 测试
    // std::string title, artist, album;

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

        struct RecordingResult { // 搜索到的recording结果
            double score{100.0}; // 最终匹配度得分
            int resultIndex{0};
            int recordingIndex{0};
        };
        std::vector<RecordingResult> recordingResults;

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

                RecordingResult recordingResult;
                recordingResult.score *= printScore;
                auto recording{results.read(resultIndex).read("recordings").read(recordingIndex)};
                auto recordingDuration{recording.read("duration").toDouble()};
                auto durationScore{1 - std::abs(recordingDuration - task.duration) / task.duration};
                if (durationScore <= 0.85) {
                    continue;
                } else {
                    recordingResult.score *= durationScore;
                } // 根据歌曲时长匹配分数

                if (task.title != std::nullopt) {
                    // 有原本的标题则进行相似度匹配
                    auto titleScore = Utils::stringSimilarity(
                        recording.read("title").toString(),
                        juce::String(task.title.value())
                    );
                    if (titleScore <= 0.7) {
                        continue;
                    }
                    recordingResult.score *= titleScore;
                }
                if (!task.artists.empty()) {
                    auto searchArtists{recording.read("artists")}; // 搜索到的艺术家列表
                    // 1. 提取线上所有的艺术家名字到一个 std::vector 中，方便后续处理
                    std::vector<juce::String> remoteArtistNames;
                    for (int i = 0; i < searchArtists.size(); i++) {
                        auto artist = searchArtists.read(i);
                        remoteArtistNames.push_back(artist.read("name").toString());
                    }

                    if (!remoteArtistNames.empty()) {
                        double totalArtistScore = 0.0;

                        // 2. 遍历本地的每一个艺术家，去线上列表中找“最相似”的一个
                        for (const auto& localArtistStr : task.artists) {
                            juce::String localArtist(localArtistStr);
                            double bestMatchForThisArtist = 0.0;

                            for (const auto& remoteName : remoteArtistNames) {
                                // 复用你现成的 Levenshtein 打分函数
                                double sim = Utils::stringSimilarity(localArtist, remoteName);
                                if (sim > bestMatchForThisArtist) {
                                    bestMatchForThisArtist = sim;
                                }
                            }
                            // 累加本地每个艺术家的最高得分
                            totalArtistScore += bestMatchForThisArtist;
                        }

                        // 3. 计算基础得分：本地歌手匹配的平均分 (范围 0.0 ~ 1.0)
                        double baseArtistScore = totalArtistScore / task.artists.size();

                        // 4. (可选但推荐) 引入数量差异的轻微惩罚
                        // 场景 A: 本地有 [周杰伦]，线上有 [周杰伦, 林迈可]。基础分为 1.0
                        // (完全命中)，但我们略微扣一点分，因为线上信息更多。 场景 B: 本地有 [A, B,
                        // C]，线上只有 [A]。这种情况应该重罚。
                        double sizeRatio =
                            (double)std::min(task.artists.size(), remoteArtistNames.size()) /
                            (double)std::max(task.artists.size(), remoteArtistNames.size());

                        // 权重分配：80%看重匹配度，20%看重数量是否一致 (权重比例你可以自己调)
                        double finalArtistScore = (baseArtistScore * 0.8) + (sizeRatio * 0.2);

                        // 5. 乘入总分
                        recordingResult.score *= finalArtistScore;

                    } else {
                        // 如果线上没有返回艺术家，适当降分
                        recordingResult.score *= 0.5;
                    }
                }
                recordingResult.resultIndex = resultIndex;
                recordingResult.recordingIndex = recordingIndex;

                recordingResults.push_back(recordingResult);
            }
        }
        // 得到最高分的recording结果
        auto bestRecordingIt = std::max_element(
            recordingResults.begin(),
            recordingResults.end(),
            [](const RecordingResult& a, const RecordingResult& b) {
                return a.score < b.score; // 比较：a < b 则 a 更小
            }
        );

        if (bestRecordingIt == recordingResults.end()) return;
        auto releases{results.read(bestRecordingIt->resultIndex)
                          .read("recordings")
                          .read(bestRecordingIt->recordingIndex)
                          .read("releases")};

        struct ReleaseResult {
            int releaseindex{0};
            double score{100.0};
        };
        std::vector<ReleaseResult> releaseResults;

        for (int releaseIndex = 0; releaseIndex < releases.size(); releaseIndex++) {
            auto remoteAlbumTitle{releases.read(releaseIndex).read("title")};
            if (task.album != std::nullopt) {
                auto albumScore{
                    Utils::stringSimilarity(task.album.value(), remoteAlbumTitle.toString())
                };
                if (albumScore <= 0.7) continue;
                ReleaseResult releaseResult;
                releaseResult.releaseindex = releaseIndex;
                releaseResult.score = albumScore;
                releaseResults.push_back(releaseResult);
            }
        }

        auto bestReleaseIt = std::max_element(
            releaseResults.begin(),
            releaseResults.end(),
            [](const ReleaseResult& a, const ReleaseResult& b) {
                return a.score < b.score; // 比较：a < b 则 a 更小
            }
        );
        if (bestReleaseIt == releaseResults.end()) return;

        auto finalRecording{results.read(bestRecordingIt->resultIndex)
                                .read("recordings")
                                .read(bestRecordingIt->recordingIndex)};

        std::string finalTitle{finalRecording.read("title").toString().toStdString()};

        auto finalArtists{finalRecording.read("artists")};

        std::vector<std::string> artistsVec;
        for (int i = 0; i < finalArtists.size(); i++) {
            auto artist{finalArtists.read(i).read("name").toString().toStdString()};
            artistsVec.push_back(artist);
        }
        auto finalArtistStr{DllUtils::tagVector2String(artistsVec)};

        auto finalRelease{finalRecording.read("releases").read(bestReleaseIt->releaseindex)};

        auto finalAlbumStr{finalRelease.read("title").toString().toStdString()};

        auto finalAlbumArtists{finalRelease.read("artists")};
        std::string finalAlbumArtistsStr;
        for (int i = 0; i < finalAlbumArtists.size(); i++) {
            auto name{finalAlbumArtists.read(i).read("name").toString().toStdString()};
            if (isVariousArtists(name)) {
                // 如果是群星等名字不能发给apple做搜索，否则极大降低成功率
                finalAlbumArtistsStr += name;
                finalAlbumArtistsStr += " ";
            }
        }
        cpr::Response coverSearch = cpr::Get(
            cpr::Url{"https://itunes.apple.com/search"},
            cpr::Parameters{
                {"term", finalAlbumArtistsStr + finalAlbumStr},
                {"media", "music"},
                {"entity", "album"},
                {"limit", "1"}
            },
            cpr::Timeout{10000}
        );

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