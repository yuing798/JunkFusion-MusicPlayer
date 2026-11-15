#include "./PluginScanProtocol.hpp"

#include <vector>

namespace PluginHost {

    namespace {
        // 文件级匿名命名空间里的两个纯函数只服务于本文件的编码/解码，
        // 不对外暴露，也不会被别的 TU 看到（不违反「不在 cpp 顶层加临时工具函数」
        // 那条规范的本意：那条禁的是"随手放在顶层、跨文件可见的散装函数"）。
        std::string joinFields(const std::vector<std::string>& fields) {
            std::string line{kProtocolPrefix};
            for (const auto& field : fields) {
                line += '\t';
                line += field;
            }
            return line;
        }

        std::vector<std::string> splitTabs(std::string_view line) {
            std::vector<std::string> parts;
            std::size_t begin = 0;
            while (true) {
                const auto end = line.find('\t', begin);
                if (end == std::string_view::npos) {
                    parts.emplace_back(line.substr(begin));
                    break;
                }
                parts.emplace_back(line.substr(begin, end - begin));
                begin = end + 1;
            }
            return parts;
        }
    } // namespace

    std::string sanitizeField(std::string_view text) {
        std::string out{text};
        for (auto& ch : out) {
            if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
        }
        return out;
    }

    std::string encodeOk(int index, const Record& record) {
        std::vector<std::string> fields;
        fields.emplace_back("OK");
        fields.emplace_back(std::to_string(index));
        fields.emplace_back(std::to_string(record.ordinal));
        fields.emplace_back(categoryToString(record.type));
        fields.emplace_back(ruleToString(record.rule));
        fields.emplace_back(std::to_string(record.meta.numInputs));
        fields.emplace_back(std::to_string(record.meta.numOutputs));
        fields.emplace_back(sanitizeField(record.meta.identifier));
        fields.emplace_back(sanitizeField(record.meta.name));
        fields.emplace_back(sanitizeField(record.meta.vendor));
        fields.emplace_back(sanitizeField(record.meta.category));
        return joinFields(fields);
    }

    std::string encodeEnd(int index) {
        std::vector<std::string> fields;
        fields.emplace_back("END");
        fields.emplace_back(std::to_string(index));
        return joinFields(fields);
    }

    std::string encodeFail(int index, std::string_view reason) {
        std::vector<std::string> fields;
        fields.emplace_back("FAIL");
        fields.emplace_back(std::to_string(index));
        fields.emplace_back(sanitizeField(reason));
        return joinFields(fields);
    }

    std::string encodeDone(int processed) {
        std::vector<std::string> fields;
        fields.emplace_back("DONE");
        fields.emplace_back(std::to_string(processed));
        return joinFields(fields);
    }

    std::string encodeLog(std::string_view message) {
        std::vector<std::string> fields;
        fields.emplace_back("LOG");
        fields.emplace_back(sanitizeField(message));
        return joinFields(fields);
    }

    ScanEvent decodeLine(std::string_view line) {
        ScanEvent event;

        const auto parts = splitTabs(line);
        if (parts.size() < 2) return event;
        if (parts[0] != kProtocolPrefix) return event;

        const std::string& kind = parts[1];

        auto toInt = [](const std::string& text, int fallback) {
            try {
                return std::stoi(text);
            } catch (...) {
                return fallback;
            }
        };

        if (kind == "OK" && parts.size() >= 12) {
            event.kind = ScanEvent::Kind::ok;
            event.index = toInt(parts[2], -1);
            event.record.ordinal = toInt(parts[3], 0);
            categoryFromString(parts[4], event.record.type);
            event.record.meta.numInputs = toInt(parts[6], 0);
            event.record.meta.numOutputs = toInt(parts[7], 0);
            event.record.meta.identifier = parts[8];
            event.record.meta.name = parts[9];
            event.record.meta.vendor = parts[10];
            event.record.meta.category = parts[11];

            // rule 只是日志证据，解错了也不影响入库，按 none 兜底即可
            const std::string& rule = parts[5];
            if (rule == "isInstrument") event.record.rule = CategoryRule::isInstrument;
            else if (rule == "category") event.record.rule = CategoryRule::categoryHint;
            else if (rule == "no-input") event.record.rule = CategoryRule::noInput;
            else if (rule == "category-fx") event.record.rule = CategoryRule::categoryFx;
            else if (rule == "has-input") event.record.rule = CategoryRule::hasInput;
            else event.record.rule = CategoryRule::none;

            return event;
        }

        if (kind == "END" && parts.size() >= 3) {
            event.kind = ScanEvent::Kind::end;
            event.index = toInt(parts[2], -1);
            return event;
        }

        if (kind == "FAIL" && parts.size() >= 4) {
            event.kind = ScanEvent::Kind::fail;
            event.index = toInt(parts[2], -1);
            event.reason = parts[3];
            return event;
        }

        if (kind == "DONE" && parts.size() >= 3) {
            event.kind = ScanEvent::Kind::done;
            event.processed = toInt(parts[2], 0);
            return event;
        }

        if (kind == "LOG" && parts.size() >= 3) {
            event.kind = ScanEvent::Kind::log;
            event.reason = parts[2];
            return event;
        }

        return event;
    }

} // namespace PluginHost
