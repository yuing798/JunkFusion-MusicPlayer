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
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
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

juce::String OnlineGetMatedata::normalizeArtistName(const juce::String& name) {
    juce::String s = name.trim().toLowerCase();

    // 将各种 Unicode 连字符（如 ‐, –, —）统一替换为标准 ASCII 短横线 '-'
    s = s.replaceCharacters(
        juce::CharPointer_UTF8("\xE2\x80\x90-\xE2\x80\x93\xE2\x80\x94\xE2\x88\x92"),
        juce::CharPointer_UTF8("-----")
    );

    // 替换全角空格（0x3000）为普通半角空格
    s = s.replace(juce::String::charToString(0x3000), " ");

    return s;
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

    spdlog::get(LogDllID)->debug(
        "已有数据内容为:songId:{},title:{},artists:{},album:{}",
        task.songId,
        task.title.toStdString(),
        task.artists.joinIntoString(" / ").toStdString(),
        task.album.toStdString()
    );

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
                    if (titleScore > 0.7) {
                        recordingResult.score *= (1.0 + titleScore);
                    } else {
                        recordingResult.score *= 0.5;
                    }
                    // recordingResult.score *= titleScore;
                }
                if (!task.artists.isEmpty()) {
                    auto searchArtists{recording.read("artists")}; // 搜索到的艺术家列表

                    juce::StringArray remoteArtistNames;
                    for (int i = 0; i < searchArtists.size(); i++) {
                        auto artist = searchArtists.read(i);
                        remoteArtistNames.add(artist.read("name").toString());
                    }

                    if (!remoteArtistNames.isEmpty()) {
                        double totalArtistScore = 0.0;

                        // 遍历本地每个艺术家，在远端寻找最优解
                        for (const auto& localArtistStr : task.artists) {
                            juce::String localNormalized = normalizeArtistName(localArtistStr);
                            double bestMatchForThisArtist = 0.0;

                            for (const auto& remoteName : remoteArtistNames) {
                                juce::String remoteNormalized = normalizeArtistName(remoteName);

                                // A. 基础字符串相似度（Levenshtein）
                                double sim =
                                    OtherUtils::stringSimilarity(localNormalized, remoteNormalized);

                                // B. 跨语种/音译启发式处理（如 周杰伦 vs Jay Chou, 字符集交集为 0）
                                if (sim < 0.2) {
                                    bool isLocalASCII = localNormalized.containsOnly(
                                        "abcdefghijklmnopqrstuvwxyz0123456789 -_."
                                    );
                                    bool isRemoteASCII = remoteNormalized.containsOnly(
                                        "abcdefghijklmnopqrstuvwxyz0123456789 -_."
                                    );

                                    // 如果一方是纯英文/罗马音，另一方是非英文（中文/日文/韩文等），大概率是音译或跨国别名
                                    if (isLocalASCII != isRemoteASCII) {
                                        sim = 0.6; // 给予中立的跨语言预估分，防止判定为 0 分
                                    }
                                }

                                if (sim > bestMatchForThisArtist) {
                                    bestMatchForThisArtist = sim;
                                }
                            }
                            // 累加本地每个艺术家的最高得分
                            totalArtistScore += bestMatchForThisArtist;
                        }

                        // 计算平均匹配分 (0.0 ~ 1.0)
                        double baseArtistScore = totalArtistScore / task.artists.size();

                        // 计算数量比率 (0.0 ~ 1.0)
                        double sizeRatio =
                            (double)std::min(task.artists.size(), remoteArtistNames.size()) /
                            (double)std::max(task.artists.size(), remoteArtistNames.size());

                        // 综合原始得分
                        double rawArtistScore = (baseArtistScore * 0.8) + (sizeRatio * 0.2);

                        // C. 【关键修正】保底平滑映射 (Floor Scaling)
                        // 将 rawArtistScore [0.0 ~ 1.0] 线性缩放映射至 [0.5 ~ 1.0]
                        // 效果：即使名字完全不匹配(0.0)，也只扣除 50% 权重，保留 0.5
                        // 的保底分，绝不抹杀 AcoustID 的指纹匹配成果
                        double safeMultiplier = 0.5 + (rawArtistScore * 0.5);

                        // 乘入总分
                        recordingResult.score *= safeMultiplier;

                    } else {
                        // 线上完全没有返回艺术家信息时，仅扣除 20% 分数
                        recordingResult.score *= 0.8;
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
            auto country{release.read("country").toString().toUpperCase()};

            ReleaseResult releaseResult;
            releaseResult.releaseindex = releaseIndex;

            if (task.album.isNotEmpty()) {
                auto albumScore{
                    OtherUtils::stringSimilarity(task.album, remoteAlbumTitle.toString())
                };
                if (albumScore > 0.7) {
                    releaseResult.score *= (1.0 + albumScore);
                } else {
                    releaseResult.score *= 0.5; // 这一步是为了防止不同语言但是又是同一张专辑的情况
                }
                // releaseResult.score *= albumScore;
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
            // spdlog::get(LogDllID)->debug(
            //     "获取专辑艺术家返回:{}",
            //     finalAlbumArtists.toString().toStdString()
            // );

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

            // spdlog::get(LogDllID)->debug("准备下载高清封面: {}", coverURL.value());

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

        bool foundValue{false}; // 是否联网搜索到了想要的内容
        std::string sqlStr = "UPDATE songs SET ";
        juce::StringArray writeArray;
        if (task.title.isEmpty()) {
            if (finalTitle.isNotEmpty()) {
                writeArray.add("title = :title");
                foundValue = true;
            }
        }
        if (task.artists.isEmpty()) {
            if (!finalArtistsArr.isEmpty()) {
                writeArray.add("artists = :artists");
                foundValue = true;
            }
        }
        if (task.album.isEmpty()) {
            if (finalAlbum.isNotEmpty()) {
                writeArray.add("album = :album");
                foundValue = true;
            }
        }
        if (task.needCover) {
            if (finalCoverHash.isNotEmpty()) {
                writeArray.add("hash = :hash");
                foundValue = true;
            }
        }
        if (foundValue == false) {
            return;
        }
        sqlStr += writeArray.joinIntoString(",").toStdString();
        sqlStr += " WHERE songId = :songId";

        try {
            spdlog::get(LogDllID)->debug("最终联网搜索的sql语句为:{}", sqlStr);
            SQLite::Statement sql{db, sqlStr};
            if (task.title.isEmpty()) {
                if (finalTitle.isNotEmpty()) {
                    sql.bind(":title", finalTitle.toStdString());
                }
            }
            if (task.artists.isEmpty()) {
                if (!finalArtistsArr.isEmpty()) {
                    sql.bind(":artists", juce::JSON::toString(finalArtistsArr).toStdString());
                }
            }
            if (task.album.isEmpty()) {
                if (finalAlbum.isNotEmpty()) {
                    sql.bind(":album", finalAlbum.toStdString());
                }
            }
            if (task.needCover) {
                if (finalCoverHash.isNotEmpty()) {
                    sql.bind(":hash", finalCoverHash.toStdString());
                }
            }
            sql.bind(":songId", task.songId);
            sql.exec();
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug("联网搜索写入数据库失败:{}", e.what());
        }

        juce::var resultObj{new juce::DynamicObject()};

        if (foundValue) {
            if (dllManager::getInstance().getSongsManager().OnUpdateSongInfo)
                dllManager::getInstance().getSongsManager().OnUpdateSongInfo(task.songId);
        }
    }
}