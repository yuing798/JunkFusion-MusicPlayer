#include "./OnlineGetMatedata.hpp"
#include "../dllUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <SQLiteCpp/Database.h>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <mutex>
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
        {
            std::lock_guard<std::mutex> lock{mtx};
            if (mTaskQueue.empty()) {
                wait(-1);
                continue;
            }
            task = std::move(mTaskQueue.front());
            mTaskQueue.pop();
        }
        processSingleRequest(task);
    }
}
void OnlineGetMatedata::processSingleRequest(OnlineGetMatedata::Task task) {}

void OnlineGetMatedata::searchDataByText(std::string title, std::string album, std::string artist) {

    // jassert(title.empty() || album.empty() || artist.empty());

    // // 数据清洗：对传入的所有字符串进行 Lucene 转义
    // std::string safe_title = DllUtils::escapeLucene(title);
    // std::string safe_album = DllUtils::escapeLucene(album);
    // std::string safe_artist = DllUtils::escapeLucene(artist);

    // // 组装 Lucene 查询语句 (使用双引号包裹，保证是短语精确匹配)
    // std::string query_str = "recording:\"" + safe_title + "\"";

    // query_str += " AND artist:\"" + safe_artist + "\"";
    // query_str += " AND release:\"" + safe_album + "\"";

    // // std::cout << "\n[网络请求] 最终组装的查询语句: " << query_str << std::endl;

    // // 3. 使用 CPR 发起 GET 请求 (URL 编码部分交由 cpr::Parameters 自动处理)
    // cpr::Response res = cpr::Get(
    //     cpr::Url{"https://musicbrainz.org/ws/2/recording/"},
    //     cpr::Parameters{
    //         {"query", query_str},
    //         {"fmt", "json"},
    //         {"limit", "3"} // 拿前3个结果即可，通常第一个就是最准的
    //     },
    //     cpr::Header{{"User-Agent", "JunkFusion/1.0.0 ( yusekx@gmail.com )"}},
    //     cpr::Timeout{5000} // 5秒超时
    // );

    // // 4. 处理服务器响应
    // if (res.status_code == 200) {
    //     // 网络通信状态码
    //     /*
    //     1xx	信息响应	“正在处理，稍等”	极少遇到，忽略即可
    //     2xx	成功响应	“成了！”	继续解析返回的 JSON/音频数据
    //     3xx	重定向响应	“地址变了，去那边找”	cpr 默认会自动跟随重定向，你无感知
    //     4xx	客户端错误	“你（客户端）搞错了”	立即停止解析数据，检查请求参数或 Header
    //     5xx	服务器错误	“服务器（MusicBrainz）宕机了”	立即停止解析，稍后重试
    //     */
    //     try {
    //         auto obj{juce::JSON::fromString(res.text)};
    //         auto objPtr{obj.getDynamicObject()};
    //         jassert(objPtr);

    //         if (j.contains("recordings") && !j["recordings"].empty()) {
    //             // 取匹配度最高的第一个结果
    //             const auto& best_match = j["recordings"][0];

    //             int score = best_match.value("score", 0);
    //             std::cout << "[解析成功] 找到最佳匹配！可信度(Score): " << score << "%"
    //                       << std::endl;

    //             if (score < 80) {
    //                 std::cout << "  -> 警告：可信度低于80%，数据可能存在偏差。" << std::endl;
    //             }

    //             // 提取歌曲 MBID 和正式名称
    //             std::string mbid = best_match["id"].get<std::string>();
    //             std::string real_title = best_match.value("title", "未知名称");
    //             std::cout << "  1. 歌曲 MBID: " << mbid << std::endl;
    //             std::cout << "  2. 官方曲名: " << real_title << std::endl;

    //             // 提取艺人名称 (MusicBrainz 的 artist-credit 通常是个数组)
    //             if (best_match.contains("artist-credit") && !best_match["artist-credit"].empty())
    //             {
    //                 std::string real_artist =
    //                     best_match["artist-credit"][0]["name"].get<std::string>();
    //                 std::cout << "  3. 官方艺人: " << real_artist << std::endl;
    //             }

    //             // 提取专辑信息
    //             if (best_match.contains("releases") && !best_match["releases"].empty()) {
    //                 const auto& release = best_match["releases"][0];

    //                 std::cout << "  4. 专辑名称: " << release.value("title", "未知") <<
    //                 std::endl; std::cout << "  5. 发行日期: " << release.value("date", "未知") <<
    //                 std::endl;

    //                 // 提取光盘号(Disc)和音轨号(Track)
    //                 if (release.contains("media") && !release["media"].empty()) {
    //                     const auto& media = release["media"][0];
    //                     std::cout << "  6. 碟号(Disc): " << media.value("position", 1) <<
    //                     std::endl;

    //                     if (media.contains("track") && !media["track"].empty()) {
    //                         std::cout
    //                             << "  7. 音轨(Track): " << media["track"][0].value("number", "1")
    //                             << std::endl;
    //                     }
    //                 }
    //             }
    //         } else {
    //             std::cout << "搜索完成，但在 MusicBrainz 库中未找到结果。" << std::endl;
    //         }
    //     } catch (const json::parse_error& e) {
    //         std::cerr << "JSON 解析失败: " << e.what() << std::endl;
    //     }
    // } else if (r.status_code == 503) {
    //     std::cerr << "错误：触发了 503 限流！请检查你的请求频率是否遵守了 1秒/次 的规则。"
    //               << std::endl;
    // } else {
    //     std::cerr << "网络请求失败！HTTP 状态码: " << r.status_code << std::endl;
    //     std::cerr << "错误信息: " << r.error.message << std::endl;
    // }
}
