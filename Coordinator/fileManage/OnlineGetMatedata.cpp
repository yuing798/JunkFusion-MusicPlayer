#include "./OnlineGetMatedata.hpp"
#include "../dllManager.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <SQLiteCpp/Database.h>
#include <algorithm>
#include <cpr/api.h>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <cstddef>
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
    : juce::Thread("OnlineGetMatedata"), db(db) {
    systemCountry = juce::SystemStats::getUserRegion().toUpperCase();
}

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

std::optional<std::string> OnlineGetMatedata::searchCoverURL(std::string term, std::string entity) {
    spdlog::get(LogDllID)->debug("iTunes 搜索 term: {}, entity: {}", term, entity);
    cpr::Response coverSearch = cpr::Get(
        cpr::Url{"https://itunes.apple.com/search"},
        cpr::Parameters{{"term", term}, {"media", "music"}, {"entity", entity}, {"limit", "1"}},
        cpr::Timeout{10000}
    );

    if (coverSearch.status_code == 200) {
        Yvar coverSearchRes{juce::JSON::fromString(juce::String(coverSearch.text))};
        if (coverSearchRes.hasProperty("resultCount") &&
            coverSearchRes.read("resultCount").toInt() > 0) {
            auto firstResult = coverSearchRes.read("results").read(0);
            return firstResult.read("artworkUrl100").toString().toStdString();
        }
    }
    return std::nullopt;
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

    // task.album = "";
    // task.artists = juce::StringArray{};
    // task.needCover = true;
    // task.title = ""; // 测试
    // std::string title, artist, album;
    juce::String finalTitle;
    juce::StringArray finalArtistsArr;
    juce::String finalAlbum;
    juce::String finalCoverHash;

    // 发起 POST 请求
    cpr::Response res = cpr::Post(
        cpr::Url{"https://api.acoustid.org/v2/lookup"},
        // cpr::Payload 会自动将参数放在 HTTP Body 中以 POST 表单发出
        cpr::Payload{
            {"client", AcoustIdKey},
            {"meta", "recordings + releases"},
            {"duration", std::to_string((int)task.duration)},
            {"fingerprint", task.print},
            {"format", "json"}
        },
        cpr::Timeout{5000} // 5秒超时
    );

    if (res.status_code == 200) {
        // spdlog::get(LogDllID)->debug("得到网络请求返回的表单数据:{}", res.text);

        struct RecordingResult { // 搜索到的recording结果
            double score{100.0}; // 最终匹配度得分
            int resultIndex{0};
            int recordingIndex{0};
        };
        std::vector<RecordingResult> recordingResults;

        Yvar v{juce::JSON::fromString(juce::String(res.text))};
        auto results{v.read("results")};

        for (int resultIndex = 0; resultIndex < results.size(); resultIndex++) {
            double printScore{results.read(resultIndex).read("score").toDouble()};
            // 原始分数
            if (printScore <= 0.7) {
                continue;
            } // 指纹匹配度小于0.7直接退出

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

                if (task.title.isNotEmpty()) {
                    // 有原本的标题则进行相似度匹配
                    auto titleScore = OtherUtils::stringSimilarity(
                        recording.read("title").toString(),
                        task.title
                    );
                    if (titleScore <= 0.7) {
                        continue;
                    }
                    recordingResult.score *= titleScore;
                }
                if (!task.artists.isEmpty()) {
                    auto searchArtists{recording.read("artists")}; // 搜索到的艺术家列表
                    // 1. 提取线上所有的艺术家名字到一个 std::vector 中，方便后续处理
                    juce::StringArray remoteArtistNames;
                    for (int i = 0; i < searchArtists.size(); i++) {
                        auto artist = searchArtists.read(i);
                        remoteArtistNames.add(artist.read("name").toString());
                    }

                    if (!remoteArtistNames.isEmpty()) {
                        double totalArtistScore = 0.0;

                        // 2. 遍历本地的每一个艺术家，去线上列表中找“最相似”的一个
                        for (const auto& localArtistStr : task.artists) {
                            juce::String localArtist(localArtistStr);
                            double bestMatchForThisArtist = 0.0;

                            for (const auto& remoteName : remoteArtistNames) {
                                // 复用你现成的 Levenshtein 打分函数
                                double sim = OtherUtils::stringSimilarity(localArtist, remoteName);
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

        if (bestRecordingIt == recordingResults.end()) {
            spdlog::get(LogDllID)->debug("找不到合适的recording,提前返回");
            return;
        }
        auto finalRecording{results.read(bestRecordingIt->resultIndex)
                                .read("recordings")
                                .read(bestRecordingIt->recordingIndex)};
        auto releases{finalRecording.read("releases")};

        spdlog::get(LogDllID)->debug("获取到的releases内容:{}", releases.toString().toStdString());

        int bestReleaseIndex{0};
        struct ReleaseResult {
            int releaseindex{0};
            double score{100.0};
        };
        std::vector<ReleaseResult> releaseResults;
        // if (task.album.isNotEmpty()) {

        for (int releaseIndex = 0; releaseIndex < releases.size(); releaseIndex++) {
            auto release{releases.read(releaseIndex)};
            auto remoteAlbumTitle{release.read("title")};
            auto country{releases.read("country").toString().toUpperCase()};

            ReleaseResult releaseResult;
            releaseResult.releaseindex = releaseIndex;

            if (task.album.isNotEmpty()) {
                auto albumScore{
                    OtherUtils::stringSimilarity(task.album, remoteAlbumTitle.toString())
                };
                if (albumScore <= 0.7) {
                    continue;
                }
                releaseResult.score *= albumScore;
            }

            if (country == systemCountry) {
                releaseResult.score *= 1.2;
            } else if (country == "XW") {
                // 全球发行版本
                releaseResult.score *= 1.1;
            } else if (country == "XE" || country == "EU") {
                // 欧洲统一发行版本
                releaseResult.score *= 1.05;
            } else {
                releaseResult.score *= 0.95;
            }

            releaseResults.push_back(releaseResult);
        }

        auto bestReleaseIt = std::max_element(
            releaseResults.begin(),
            releaseResults.end(),
            [](const ReleaseResult& a, const ReleaseResult& b) {
                return a.score < b.score; // 比较：a < b 则 a 更小
            }
        );
        if (bestReleaseIt == releaseResults.end()) {
            spdlog::get(LogDllID)->debug("找不到合适的release，提前return");
            return;
        }
        bestReleaseIndex = bestReleaseIt->releaseindex;
        // }

        finalTitle = finalRecording.read("title").toString().toStdString();

        auto finalArtists{finalRecording.read("artists")};

        for (int i = 0; i < finalArtists.size(); i++) {
            auto artist{finalArtists.read(i).read("name").toString()};
            finalArtistsArr.add(artist);
        }

        auto finalRelease{finalRecording.read("releases").read(bestReleaseIndex)};

        finalAlbum = finalRelease.read("title").toString();

        spdlog::get(LogDllID)->debug(
            "id:{}:最终联网搜索获得的歌名:{},艺术家组合名:{},专辑名:{}",
            task.songId,
            finalTitle.toStdString(),
            finalArtistsArr.joinIntoString(" / ").toStdString(),
            finalAlbum.toStdString()
        );
        if (task.needCover) {
            // if (artistsVec.empty()) break;

            auto finalAlbumArtists{finalRelease.read("artists")};
            spdlog::get(LogDllID)->debug(
                "获取专辑艺术家返回:{}",
                finalAlbumArtists.toString().toStdString()
            );

            std::string primaryArtist;
            for (int i = 0; i < finalAlbumArtists.size(); i++) {
                auto name{finalAlbumArtists.read(i).read("name").toString().toStdString()};
                if (!isVariousArtists(name)) {
                    // 如果是群星等名字不能发给apple做搜索，否则极大降低成功率
                    primaryArtist = name;
                    break; // 搜索专辑图只需主艺术家名
                }
            }
            if (primaryArtist.empty() && !finalArtistsArr.isEmpty()) {
                primaryArtist = finalArtistsArr[0].toStdString();
            } // 如果专辑艺术家不可用的话，发送歌曲艺术家

            auto searchTerms{primaryArtist + " " + finalAlbum.toStdString()};
            auto coverURL{searchCoverURL(searchTerms, "album")};
            if (coverURL == std::nullopt) {
                searchTerms = primaryArtist + " " + finalTitle.toStdString();
                coverURL = searchCoverURL(searchTerms, "song");
                // 艺术家加上专辑名走不到就改用歌曲名称
                if (coverURL == std::nullopt) {
                    spdlog::get(LogDllID)->info("搜不到专辑图片:歌曲ID{}", task.songId);
                    return;
                }
            }

            // 替换 URL 获取 600x600 或 1000x1000 高清大图
            size_t pos = coverURL.value().find("100x100bb.jpg");
            if (pos != std::string::npos) {
                coverURL.value().replace(pos, 13, "1000x1000bb.jpg"); // 替换为目标分辨率
            }

            spdlog::get(LogDllID)->debug("准备下载高清封面: {}", coverURL.value());

            // 5. 发起第二次请求：真正下载图片数据
            cpr::Response imgRes = cpr::Get(cpr::Url{coverURL.value()}, cpr::Timeout{10000});

            if (imgRes.status_code == 200) {

                do {
                    SHA1 sha1;
                    auto hash = sha1(imgRes.text.data(), imgRes.text.size());
                    spdlog::get(LogDllID)->debug("联网搜索到图片哈希值:{}", hash);
                    finalCoverHash = juce::String(hash);

                    juce::File hashImageDir{
                        dllManager::getInstance().getSongImageDir().getChildFile(hash)
                    };

                    if (hashImageDir.exists()) {
                        break;
                    } else {
                        hashImageDir.createDirectory();
                    } // 如果这个目录已经存在，直接退出，避免保存两个相同图片

                    juce::File originalFile{hashImageDir.getChildFile("original.jpg")};

                    juce::FileOutputStream outputStream(originalFile);
                    if (outputStream.openedOk()) {
                        outputStream.write(imgRes.text.data(), imgRes.text.size());
                        outputStream.flush();
                    }

                } while (0);

            } else {
                spdlog::get(LogDllID)->warn("封面图片下载失败，状态码: {}", imgRes.status_code);
            }

        } else {
            spdlog::get(LogDllID)->error(
                "请求失败！错误码: {} | 服务器返回的错误信息: {}",
                res.status_code,
                res.text
            );
        }
        spdlog::get(LogDllID)->debug("联网搜索歌曲元数据完成，歌曲ID:{}", task.songId);

        juce::var resultObj{new juce::DynamicObject()};
        if (task.title.isEmpty()) {
            resultObj.getDynamicObject()->setProperty(SongInfoMacro::title, finalTitle);
        }
        if (task.artists.isEmpty()) {
            resultObj.getDynamicObject()->setProperty(
                SongInfoMacro::artists,
                ConvertUtils::stringArray2ArrayVar(finalArtistsArr)
            );
        }
        if (task.album.isEmpty()) {
            resultObj.getDynamicObject()->setProperty(SongInfoMacro::album, finalAlbum);
        }
        if (task.needCover) {
            resultObj.getDynamicObject()->setProperty(SongInfoMacro::hash, finalCoverHash);
        }

        if (onSearchOver) onSearchOver(ConvertUtils::object2Uint8t(resultObj));
    }
}