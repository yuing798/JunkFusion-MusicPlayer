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
        Yvar coverSearchRes = Yvar::parse(coverSearch.text);
        if (coverSearchRes.read("resultCount").toInt() > 0) {
            auto firstResult = coverSearchRes.read("results").read(0);
            return firstResult.read("artworkUrl100").toOptStr();
        }
    }
    return std::nullopt;
}

OnlineGetMatedata::Result
OnlineGetMatedata::process(std::string systemCountry, OnlineGetMatedata::Task task) {
    spdlog::get(LogDllID)->debug("开始根据指纹搜索元数据:歌曲ID:{}", task.songId);

    Result onLineResult;

    spdlog::get(LogDllID)->debug(
        "已有数据内容为:songId:{},title:{},artists:{},album:{}",
        task.songId,
        task.title.has_value() ? task.title.value() : "",
        task.artists.has_value() ? StringUtils::vectorToString(task.artists.value(), " / ") : "",
        task.album.has_value() ? task.album.value() : ""
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

        Yvar v = Yvar::parse(res.text);
        const Yvar results = v.read("results");

        for (int resultIndex = 0; resultIndex < results.size(); resultIndex++) {
            const Yvar result = results.read(resultIndex);
            double printScore{result.read("score").toDouble()};
            // 原始分数
            if (printScore <= 0.7) {
                continue;
            } // 指纹匹配度小于0.7直接退出

            const Yvar recordings = result.read("recordings");
            for (int recordingIndex = 0; recordingIndex < recordings.size(); recordingIndex++) {

                RecordingResult recordingResult;
                recordingResult.score *= printScore;
                const Yvar recording = recordings.read(recordingIndex);
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
                    const Yvar searchArtists = recording.read("artists"); // 搜索到的艺术家列表

                    std::vector<std::string> remoteArtistNames;
                    for (int i = 0; i < searchArtists.size(); i++) {
                        auto name = searchArtists.read(i).read("name").toString();
                        if (!name.empty()) remoteArtistNames.push_back(std::move(name));
                    }

                    if (!remoteArtistNames.empty()) {
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
        const Yvar finalRecording = results.read(bestRecordingIt->resultIndex)
                                        .read("recordings")
                                        .read(bestRecordingIt->recordingIndex);
        const Yvar releases = finalRecording.read("releases");

        spdlog::get(LogDllID)->debug("获取到的releases内容:{}", releases.toString());

        int bestReleaseIndex{0};
        struct ReleaseResult {
            int releaseindex{0};
            double score{100.0};
        };
        std::vector<ReleaseResult> releaseResults;
        // if (task.album.isNotEmpty()) {

        for (int releaseIndex = 0; releaseIndex < releases.size(); releaseIndex++) {
            const Yvar release = releases.read(releaseIndex);
            const Yvar remoteAlbumTitle = release.read("title");
            auto country = release.read("country").toString();
            StringUtils::toUpperCaseInPlace(country);

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

        const Yvar bestArtists = finalRecording.read("artists");
        std::vector<std::string> bestArtistsArray;

        for (int i = 0; i < bestArtists.size(); i++) {
            auto artist = bestArtists.read(i).read("name").toString();
            if (!artist.empty()) bestArtistsArray.push_back(std::move(artist));
        }

        const Yvar finalRelease = finalRecording.read("releases").read(bestReleaseIndex);

        std::string bestAlbum = finalRelease.read("title").toString();

        spdlog::get(LogDllID)->debug(
            "id:{}:最终联网搜索获得的歌名:{},艺术家组合名:{},专辑名:{}",
            task.songId,
            bestTitle,
            StringUtils::vectorToString(bestArtistsArray, " / "),
            bestAlbum
        );
        std::string bestHash;
        std::vector<std::string> bestAlbumArtists;
        const Yvar finalAlbumArtistsVar = finalRelease.read("artists");
        for (int i = 0; i < finalAlbumArtistsVar.size(); i++) {
            auto name = finalAlbumArtistsVar.read(i).read("name").toString();
            if (!name.empty()) bestAlbumArtists.push_back(std::move(name));
        }

        if (task.needCover) {

            std::string primaryArtist;
            for (int i = 0; i < bestAlbumArtists.size(); i++) {
                if (!StringUtils::isVariousArtists(bestAlbumArtists[i])) {
                    // 如果是群星等名字不能发给apple做搜索，否则极大降低成功率
                    primaryArtist = bestAlbumArtists[i];
                    break; // 搜索专辑图只需主艺术家名
                }
            }

            if (primaryArtist.empty() && !bestArtistsArray.empty()) {
                primaryArtist = bestArtistsArray[0];
            } // 如果专辑艺术家不可用的话，发送歌曲艺术家

            auto searchTerms{primaryArtist + " " + bestAlbum};
            auto coverURL{searchCoverURL(searchTerms, "album")};
            if (coverURL == std::nullopt) {
                searchTerms = primaryArtist + " " + bestTitle;
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
                    bestHash = std::string(hash);

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

        if (!task.title.has_value() && !bestTitle.empty()) {
            onLineResult.finalTitle = bestTitle;
        }
        if (!task.artists.has_value() && !bestArtistsArray.empty()) {
            onLineResult.finalArtists = bestArtistsArray;
        }
        if (!task.album.has_value() && !bestAlbum.empty()) {
            onLineResult.finalAlbum = bestAlbum;
        }
        if (task.needCover && !bestHash.empty()) {
            onLineResult.finalHash = bestHash;
        }
        if (!bestAlbumArtists.empty()) {
            onLineResult.finalAlbumArtists = bestAlbumArtists;
        }
        return onLineResult;

    } else {
        spdlog::get(LogDllID)
            ->error("请求失败！错误码: {} | 服务器返回的错误信息: {}", res.status_code, res.text);

        return {};
    }
}