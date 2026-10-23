#include "./OnlineGetMatedata.hpp"
#include "../dllManager.hpp"
#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/stringUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
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

// OnlineGetMatedata::OnlineGetMatedata(CenterDbManage* dbManage)
//     : juce::Thread("OnlineGetMatedata"), dbManage(dbManage) {
//     systemCountry = juce::SystemStats::getUserRegion().toUpperCase();
// }

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

// void OnlineGetMatedata::setTask(OnlineGetMatedata::Task task) {
//     std::lock_guard<std::mutex> lock{mtx};
//     mTaskQueue.push(std::move(task));
//     notify();
//     if (!isThreadRunning()) startThread();
// }
// void OnlineGetMatedata::run() {
//     while (!threadShouldExit()) {
//         Task task;
//         bool hasTask = false;

//         // 1. 缩小锁的作用域，仅在弹出任务时持锁
//         {
//             std::lock_guard<std::mutex> lock(mtx);
//             if (!mTaskQueue.empty()) {
//                 task = std::move(mTaskQueue.front());
//                 mTaskQueue.pop();
//                 hasTask = true;
//             }
//         } // 锁在此处自动释放

//         // 2. 根据是否有任务决定处理还是休眠
//         if (hasTask) {
//             searchDataByPrint(std::move(task)); // 在锁外执行耗时任务
//         } else {
//             // 在【无锁状态】下安全挂起等待新任务！
//             wait(-1);
//         }
//     }
// }

OnlineGetMatedata::Result
OnlineGetMatedata::process(juce::String systemCountry, OnlineGetMatedata::Task task) {
    spdlog::get(LogDllID)->debug("开始根据指纹搜索元数据:歌曲ID:{}", task.songId);

    Result result;

    spdlog::get(LogDllID)->debug(
        "已有数据内容为:songId:{},title:{},artists:{},album:{}",
        task.songId,
        task.title.has_value() ? task.title.value().toStdString() : "",
        task.artists.has_value() ? task.artists.value().joinIntoString(" / ").toStdString() : "",
        task.album.has_value() ? task.album.value().toStdString() : ""
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

                if (task.title.has_value()) {
                    // 有原本的标题则进行相似度匹配
                    auto titleScore = StringUtils::stringSimilarity(
                        recording.read("title").toString(),
                        task.title.value()
                    );
                    if (titleScore > 0.7) {
                        recordingResult.score *= (1.0 + titleScore);
                    } else {
                        recordingResult.score *= 0.5;
                    }
                    // recordingResult.score *= titleScore;
                }
                if (task.artists.has_value()) {
                    auto searchArtists{recording.read("artists")}; // 搜索到的艺术家列表

                    juce::StringArray remoteArtistNames;
                    for (int i = 0; i < searchArtists.size(); i++) {
                        auto artist = searchArtists.read(i);
                        remoteArtistNames.add(artist.read("name").toString());
                    }

                    if (!remoteArtistNames.isEmpty()) {
                        double calcResult{StringUtils::calcArtistsSimlarity(
                            task.artists.value(),
                            remoteArtistNames
                        )};

                        // 乘入总分
                        recordingResult.score *= calcResult;

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
            return {};
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

            if (task.album.has_value()) {
                auto albumScore{
                    StringUtils::stringSimilarity(task.album.value(), remoteAlbumTitle.toString())
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
            return {};
        }
        bestReleaseIndex = bestReleaseIt->releaseindex;

        auto bestTitle = finalRecording.read("title").toString();

        auto bestArtists{finalRecording.read("artists")};
        juce::StringArray bestArtistsArray;

        for (int i = 0; i < bestArtists.size(); i++) {
            auto artist{bestArtists.read(i).read("name").toString()};
            bestArtistsArray.add(artist);
        }

        auto finalRelease{finalRecording.read("releases").read(bestReleaseIndex)};

        juce::String bestAlbum = finalRelease.read("title").toString();

        spdlog::get(LogDllID)->debug(
            "id:{}:最终联网搜索获得的歌名:{},艺术家组合名:{},专辑名:{}",
            task.songId,
            bestTitle.toStdString(),
            bestArtistsArray.joinIntoString(" / ").toStdString(),
            bestAlbum.toStdString()
        );
        juce::String bestHash;
        juce::StringArray bestAlbumArtists;
        auto finalAlbumArtistsVar{finalRelease.read("artists")};
        for (int i = 0; i < finalAlbumArtistsVar.size(); i++) {
            auto name{finalAlbumArtistsVar.read(i).read("name").toString()};
            bestAlbumArtists.add(name);
        }

        if (task.needCover) {

            std::string primaryArtist;
            for (int i = 0; i < bestAlbumArtists.size(); i++) {
                if (!StringUtils::isVariousArtists(bestAlbumArtists[i].toStdString())) {
                    // 如果是群星等名字不能发给apple做搜索，否则极大降低成功率
                    primaryArtist = bestAlbumArtists[i].toStdString();
                    break; // 搜索专辑图只需主艺术家名
                }
            }

            if (primaryArtist.empty() && !bestArtistsArray.isEmpty()) {
                primaryArtist = bestArtistsArray[0].toStdString();
            } // 如果专辑艺术家不可用的话，发送歌曲艺术家

            auto searchTerms{primaryArtist + " " + bestAlbum.toStdString()};
            auto coverURL{searchCoverURL(searchTerms, "album")};
            if (coverURL == std::nullopt) {
                searchTerms = primaryArtist + " " + bestTitle.toStdString();
                coverURL = searchCoverURL(searchTerms, "song");
                // 艺术家加上专辑名走不到就改用歌曲名称
                if (coverURL == std::nullopt) {
                    spdlog::get(LogDllID)->info("搜不到专辑图片:歌曲ID{}", task.songId);
                    return {};
                }
            }

            // 替换 URL 获取 600x600 或 1000x1000 高清大图
            size_t pos = coverURL.value().find("100x100bb.jpg");
            if (pos != std::string::npos) {
                coverURL.value().replace(pos, 13, "1000x1000bb.jpg"); // 替换为目标分辨率
            }

            // 5. 发起第二次请求：真正下载图片数据
            cpr::Response imgRes = cpr::Get(cpr::Url{coverURL.value()}, cpr::Timeout{10000});

            if (imgRes.status_code == 200) {

                do {
                    SHA1 sha1;
                    auto hash = sha1(imgRes.text.data(), imgRes.text.size());
                    spdlog::get(LogDllID)->debug("联网搜索到图片哈希值:{}", hash);
                    bestHash = juce::String(hash);

                    juce::File hashImageDir{
                        dllManager::getInstance().getImageDir().getChildFile(hash)
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
        }
        spdlog::get(LogDllID)->debug("联网搜索歌曲元数据完成，歌曲ID:{}", task.songId);

        if (!task.title.has_value() && bestTitle.isNotEmpty()) {
            result.finalTitle = bestTitle;
        }
        if (!task.artists.has_value() && !bestArtistsArray.isEmpty()) {
            result.finalArtists = bestArtistsArray;
        }
        if (!task.album.has_value() && bestAlbum.isNotEmpty()) {
            result.finalAlbum = bestAlbum;
        }
        if (task.needCover && bestHash.isNotEmpty()) {
            result.finalHash = bestHash;
        }
        if (!bestAlbumArtists.isEmpty()) {
            result.finalAlbumArtists = bestAlbumArtists;
        }
        return result;

        // bool foundValue{false}; // 是否联网搜索到了想要的内容
        // std::string sqlStr = "UPDATE songs SET ";
        // juce::StringArray writeArray;
        // if (task.title.isEmpty()) {
        //     if (finalTitle.isNotEmpty()) {
        //         writeArray.add("title = :title");
        //         foundValue = true;
        //     }
        // }
        // if (task.artists.isEmpty()) {
        //     if (!finalArtistsArr.isEmpty()) {
        //         writeArray.add("artists = :artists");
        //         foundValue = true;
        //     }
        // }
        // if (task.album.isEmpty()) {
        //     if (finalAlbum.isNotEmpty()) {
        //         writeArray.add("album = :album");
        //         foundValue = true;
        //     }
        // }
        // if (task.needCover) {
        //     if (finalCoverHash.isNotEmpty()) {
        //         writeArray.add("hash = :hash");
        //         foundValue = true;
        //     }
        // }
        // if (foundValue == false) {
        //     return;
        // }
        // sqlStr += writeArray.joinIntoString(",").toStdString();
        // sqlStr += " WHERE songId = :songId";

        // dbManage->write([&](SQLite::Database& db) {
        //     try {
        //         spdlog::get(LogDllID)->debug("最终联网搜索的sql语句为:{}", sqlStr);
        //         SQLite::Statement sql{db, sqlStr};
        //         if (task.title.isEmpty()) {
        //             if (finalTitle.isNotEmpty()) {
        //                 sql.bind(":title", finalTitle.toStdString());
        //             }
        //         }
        //         if (task.artists.isEmpty()) {
        //             if (!finalArtistsArr.isEmpty()) {
        //                 sql.bind(":artists",
        //                 juce::JSON::toString(finalArtistsArr).toStdString());
        //             }
        //         }
        //         if (task.album.isEmpty()) {
        //             if (finalAlbum.isNotEmpty()) {
        //                 sql.bind(":album", finalAlbum.toStdString());
        //             }
        //         }
        //         if (task.needCover) {
        //             if (finalCoverHash.isNotEmpty()) {
        //                 sql.bind(":hash", finalCoverHash.toStdString());
        //             }
        //         }
        //         sql.bind(":songId", task.songId);
        //         sql.exec();

        //     } catch (SQLite::Exception& e) {
        //         spdlog::get(LogDllID)->debug("联网搜索写入数据库失败:{}", e.what());
        //     }

        //     juce::var resultObj{new juce::DynamicObject()};

        //     if (foundValue) {
        //         if (dllManager::getInstance().getSongsManager().OnUpdateSongInfo)
        //             dllManager::getInstance().getSongsManager().OnUpdateSongInfo(task.songId);
        //     }
        // });
    } else {
        spdlog::get(LogDllID)
            ->error("请求失败！错误码: {} | 服务器返回的错误信息: {}", res.status_code, res.text);

        return {};
    }
}