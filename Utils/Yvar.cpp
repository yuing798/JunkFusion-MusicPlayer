#include "./Yvar.hpp"

#include <cstddef>
#include <string>

// ── 写：类型用错在 Debug 下抓出来，Release 下自动修正而不是抛异常 ──────────
void Yvar::ensureObject() {
    if (value.is_object()) return;
    assert(value.is_null() && "Yvar::set(): 当前值不是 object，是不是在 array 上写了 key？");
    value = nlohmann::json::object();
}

void Yvar::ensureArray() {
    if (value.is_array()) return;
    assert(value.is_null() && "Yvar::push(): 当前值不是 array，是不是在 object 上 push？");
    value = nlohmann::json::array();
}

Yvar& Yvar::set(std::string_view key, const Yvar& v) {
    ensureObject();
    value[std::string{key}] = v.value;
    return *this;
}

Yvar& Yvar::set(std::string_view key, Yvar&& v) {
    ensureObject();
    value[std::string{key}] = std::move(v.value);
    return *this;
}

Yvar& Yvar::push(const Yvar& v) {
    ensureArray();
    value.push_back(v.value);
    return *this;
}

Yvar& Yvar::push(Yvar&& v) {
    ensureArray();
    value.push_back(std::move(v.value));
    return *this;
}

// ── 解析 / 序列化：绝不抛异常 ────────────────────────────────────────────
Yvar Yvar::parse(std::string_view text) noexcept {
    try {
        return Yvar{nlohmann::json::parse(text.begin(), text.end())};
    } catch (...) {
        return Yvar{};
    }
}

std::optional<Yvar> Yvar::tryParse(std::string_view text) noexcept {
    try {
        return Yvar{nlohmann::json::parse(text.begin(), text.end())};
    } catch (...) {
        return std::nullopt;
    }
}

std::string Yvar::dump() const {
    return value.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

// ── 可选链读取：取不到就给空值，绝不抛 ───────────────────────────────────
Yvar Yvar::read(std::string_view key) const {
    if (!value.is_object()) return Yvar{};
    const auto it = value.find(std::string{key});
    if (it == value.end()) return Yvar{};
    return Yvar{*it};
}

Yvar Yvar::read(int index) const {
    if (!value.is_array()) return Yvar{};
    if (index < 0 || index >= static_cast<int>(value.size())) return Yvar{};
    return Yvar{value.at(static_cast<std::size_t>(index))};
}

std::optional<Yvar> Yvar::find(std::string_view key) const {
    if (!value.is_object()) return std::nullopt;
    const auto it = value.find(std::string{key});
    if (it == value.end()) return std::nullopt;
    return Yvar{*it};
}

std::optional<Yvar> Yvar::find(int index) const {
    if (!value.is_array()) return std::nullopt;
    if (index < 0 || index >= static_cast<int>(value.size())) return std::nullopt;
    return Yvar{value.at(static_cast<std::size_t>(index))};
}

bool Yvar::has(std::string_view key) const {
    return value.is_object() && value.contains(std::string{key});
}

// ── 终结取值：宽松版本 ───────────────────────────────────────────────────
int64_t Yvar::itemToInt64(const nlohmann::json& item) noexcept {
    if (!item.is_number()) return 0;

    if (item.is_number_float()) {
        const double d = item.get<double>();
        // NaN / ±Inf / 超出 int64 范围的一律当 0，避免 static_cast 的未定义行为
        if (std::isnan(d) || d < -9223372036854775808.0 || d >= 9223372036854775808.0) return 0;
        return static_cast<int64_t>(d);
    }

    if (item.is_number_unsigned()) {
        const uint64_t u = item.get<uint64_t>();
        const auto limit = static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
        return u > limit ? std::numeric_limits<int64_t>::max() : static_cast<int64_t>(u);
    }

    return item.get<int64_t>();
}

double Yvar::toDouble() const {
    // 整数和浮点都是"数字"，JSON 里 210 和 210.0 没有语义区别
    if (!value.is_number()) return 0.0;
    return value.get<double>();
}

int64_t Yvar::toInt64() const { return itemToInt64(value); }

int Yvar::toInt() const {
    const int64_t v = toInt64();
    // 限位
    if (v > std::numeric_limits<int>::max()) return std::numeric_limits<int>::max();
    if (v < std::numeric_limits<int>::min()) return std::numeric_limits<int>::min();
    return static_cast<int>(v);
}

std::string Yvar::toString() const {
    if (value.is_null()) return {}; // 空值 -> ""，不是 "{}"
    if (value.is_string()) return value.get<std::string>();
    if (value.is_boolean()) return value.get<bool>() ? "true" : "false";
    return dump(); // 数字 / object / array
}

std::vector<std::string> Yvar::toStringArray() const {
    std::vector<std::string> result;
    if (!value.is_array()) return result;
    result.reserve(value.size());
    for (const auto& item : value) {
        if (item.is_string()) result.push_back(item.get<std::string>());
    }
    return result;
}

bool Yvar::toBool() const {
    if (value.is_boolean()) return value.get<bool>();
    if (value.is_number()) return toInt64() != 0;
    if (value.is_string()) {
        const auto text = value.get<std::string>();
        return text == "true" || text == "1";
    }
    return false;
}

int Yvar::size() const {
    // 标量在 nlohmann 里 size() 是 1，这里统一成 0，让 for 循环安全地不执行
    if (!value.is_array() && !value.is_object()) return 0;
    return static_cast<int>(value.size());
}

// ── 终结取值：严格版本 ───────────────────────────────────────────────────
void Yvar::throwTypeError(const char* expected) const {
    throw TypeError{
        std::string{"Yvar 取值失败: 期望 "} + expected + "，实际是 " + value.type_name()
    };
}
