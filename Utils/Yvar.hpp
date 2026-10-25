#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace YvarDetail {

    /**
     * @brief 允许直接写进 json 的类型白名单
     *
     * 这里只放"写进去以后语义不会被悄悄改变"的类型。
     * 被挡在编译期的几个坑:
     *  - char                        : 存进去变成数字 97，而不是 "a"
     *  - signed char / unsigned char : 同上，uint8_t 的字节语义和数字语义容易混
     *  - 枚举                        : 会静默退化成整数，改枚举值时历史数据集体失效
     *  - 指针 / void* / nullptr      : 悬垂指针、往 json 里写空指针都是运行时才炸
     *  - long double 等非白名单类型
     *
     * std::vector<T> 由 set/push_back 的专门重载处理(元素同样走这个白名单)。
     */
    template <typename T> constexpr bool isSafeValue() {
        /**
         * @brief decay:去掉引用、const/volatile

         数组 → 指针（const char[4] → const char*）

         函数 → 函数指针
         *
         */
        using U = std::decay_t<T>; // decay: 字符串字面量 char[4] -> char*，否则会被误杀

        if constexpr (std::is_same_v<U, bool>) {
            return true;
        } else if constexpr (std::is_same_v<U, std::string> ||
                             std::is_same_v<U, std::string_view>) {
            return true;
        } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
            return true;
        } else if constexpr (std::is_enum_v<U>) {
            return false;
        } else if constexpr (std::is_floating_point_v<U>) {
            return std::is_same_v<U, float> || std::is_same_v<U, double>;
        } else if constexpr (std::is_integral_v<U>) {
            return !std::is_same_v<U, bool> && !std::is_same_v<U, char> &&
                   !std::is_same_v<U, signed char> && !std::is_same_v<U, unsigned char> &&
                   !std::is_same_v<U, wchar_t> && !std::is_same_v<U, char16_t> &&
                   !std::is_same_v<U, char32_t> && !std::is_same_v<U, char8_t>;
        } else {
            return false;
        }
    }

    /** @brief 支持 get<T>() / toVector<T>() 的元素类型 */
    template <typename T> constexpr bool isSupportedTerminal() {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, std::string> || std::is_same_v<U, bool> ||
                      std::is_same_v<U, std::vector<std::string>>) {
            return true;
        } else if constexpr (std::is_arithmetic_v<U>) {
            return !std::is_same_v<U, bool> && !std::is_same_v<U, char> &&
                   !std::is_same_v<U, signed char> && !std::is_same_v<U, unsigned char>;
        } else {
            return false;
        }
    }

    /**
     * @brief 把白名单里的值转成 json，顺带处理 string_view 和空指针
     *
     * 注意：这里**必须**用圆括号 `nlohmann::json(v)`，不能用花括号 `nlohmann::json{v}`。
     * 因为 basic_json 有一个 `basic_json(std::initializer_list<basic_json>, ...)` 构造函数，
     * 而 C++ 规定列表初始化(花括号)时 initializer_list 构造函数优先，结果是:
     *     nlohmann::json{0}          -> [0]        数组！不是数字 0
     *     nlohmann::json{"abc"}      -> ["abc"]    数组！不是字符串
     *     nlohmann::json{3.14}       -> [3.14]
     * 圆括号是普通重载决议，会正确选到 basic_json(int) / basic_json(string) 等。
     */
    template <typename T> nlohmann::json toJsonValue(const T& v) {
        using U = std::decay_t<T>;

        if constexpr (std::is_same_v<U, std::string_view>) {
            return nlohmann::json(std::string{v});
        } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
            // 空指针写成 null，而不是让 std::string(nullptr) 在运行时炸
            return v == nullptr ? nlohmann::json() : nlohmann::json(std::string(v));
        } else {
            return nlohmann::json(v);
        }
    }

} // namespace YvarDetail

/**
 * @brief 可选链(optional chaining)风格的安全 JSON 读写
 *
 * ── 设计目的只有两个 ──────────────────────────────────────────────────
 * 1. 安全读: 取不到值不抛异常、也不返回"必须判断的东西"，而是返回**空值**，
 *    可以像 JS 的 `?.` 一样一路链下去:
 *
 *        auto name = v.read("results").read(0).read("artists").read(0).read("name").toString();
 *        // 中间任何一环缺失 -> name == ""，一个 if 都不用写
 *
 * 2. 安全写: set/push 的写入类型被白名单(YvarDetail::isSafeValue)约束，
 *    nlohmann 那套"什么都能隐式转换、编译期全过、运行时才炸"的坑
 *    (char 存成 97、枚举退化成整数、空指针写进 json、越界写)
 *    全部变成编译错误。
 *
 * ── 取值三段式 ────────────────────────────────────────────────────────
 * | 写法        | 缺失 / 类型不符时          | 用在哪                       |
 * |-------------|----------------------------|------------------------------|
 * | read 链     | 返回空 Yvar，链继续        | 中间层导航、第三方 API       |
 * | toXxx()     | 安全默认值(""、0、false)   | 可以缺省的字段               |
 * | getOpt<T>() | std::nullopt               | 末尾一条 if 就能处理         |
 * | get<T>()    | 抛 Yvar::TypeError         | 必需字段、内部协议、数据损坏 |
 *
 * get<T>() 只在**真正必须存在**的字段上用，不要拿它读第三方 API 的稀疏字段。
 */
class Yvar {
public:
    /** @brief get<T>() 在"缺失或类型不符"时抛这个 */
    class TypeError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

private:
    nlohmann::json value;

    void ensureObject();
    void ensureArray();

    // [[noreturn]]指的是调用之后不会继续执行调用点后面的代码
    [[noreturn]] void throwTypeError(const char* expected) const;

    /** @brief 把单个 json 数字收敛成 int64，NaN/超范围返回 0，不制造未定义行为 */
    static int64_t itemToInt64(const nlohmann::json& item) noexcept;

    template <typename T> bool typeMatches() const {
        if constexpr (std::is_same_v<T, std::string>) {
            return value.is_string();
        } else if constexpr (std::is_same_v<T, bool>) {
            return value.is_boolean();
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            return value.is_array();
        } else {
            return value.is_number();
        }
    }

    template <typename T> static const char* expectedName() {
        if constexpr (std::is_same_v<T, std::string>) {
            return "string";
        } else if constexpr (std::is_same_v<T, bool>) {
            return "boolean";
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            return "array";
        } else {
            return "number";
        }
    }

    /**
     * @brief 只接受 nlohmann::json 本身，不接受"能隐式转成 json 的任意类型"
     *
     * 如果把这里写成 `Yvar(nlohmann::json)`，那么 `Yvar y = 97;`、`Yvar y = 'a';`
     * 都会编译通过——安全写在类型层面就漏了。这里用约束模板把口子堵死。
     */
    template <
        typename T,
        std::enable_if_t<std::is_same_v<std::decay_t<T>, nlohmann::json>, int> = 0>
    Yvar(T&& v) : value(std::forward<T>(v)) {} // NOLINT: 允许 nlohmann::json 隐式转 Yvar

public:
    // ── 构造 / 解析 ───────────────────────────────────────────────────
    /** @brief 默认是空值: 读任意键都安全，第一次 set/push 时自动变成 object/array */
    Yvar() noexcept : value(nullptr) {}

    static Yvar object() { return Yvar{nlohmann::json::object()}; }
    static Yvar array() { return Yvar{nlohmann::json::array()}; }

    template <typename T> static Yvar fromVector(const std::vector<T>& items) {
        static_assert(YvarDetail::isSafeValue<T>(), "Yvar::fromVector<T>: 不支持的元素类型");
        Yvar result = array();
        for (const auto& item : items)
            result.value.push_back(YvarDetail::toJsonValue(item));
        return result;
    }

    /**
     * @brief optional 有值写值、没值写 null
     *
     * 同样必须用圆括号：`nlohmann::json{item.value()}` 会把标量包成单元素数组。
     */
    template <typename T> static Yvar fromOptional(const std::optional<T>& item) {
        return item.has_value() ? Yvar{nlohmann::json(item.value())} : Yvar{};
    }

    /** @brief 解析失败返回空值，保证后续可选链依然成立(不抛异常) */
    static Yvar parse(std::string_view text) noexcept;

    /** @brief 需要明确知道"解析成功没有"时用这个 */
    static std::optional<Yvar> tryParse(std::string_view text) noexcept;

    // ── 写 ────────────────────────────────────────────────────────────
    Yvar& set(std::string_view key, const Yvar& v);
    Yvar& set(std::string_view key, Yvar&& v);

    template <typename T> Yvar& set(std::string_view key, const T& v) {
        static_assert(
            YvarDetail::isSafeValue<T>(),
            "Yvar::set: 不支持的写入类型。只接受 bool / 整数 / float / double / std::string / "
            "std::string_view / const char* / std::vector / Yvar；"
            "char、枚举、指针请先 static_cast 成明确的类型再写。"
        );
        ensureObject();
        value[std::string{key}] = YvarDetail::toJsonValue(v);
        return *this;
    }

    template <typename T> Yvar& set(std::string_view key, const std::vector<T>& items) {
        static_assert(YvarDetail::isSafeValue<T>(), "Yvar::set: 不支持的 vector 元素类型");
        ensureObject();
        value[std::string{key}] = fromVector(items).value;
        return *this;
    }

    Yvar& push(const Yvar& v);
    Yvar& push(Yvar&& v);

    template <typename T> Yvar& push_back(const T& v) {
        static_assert(YvarDetail::isSafeValue<T>(), "Yvar::push_back: 不支持的写入类型");
        ensureArray();
        value.push_back(YvarDetail::toJsonValue(v));
        return *this;
    }

    // 把整个数组当成一个元素塞进去，不会扁平化
    template <typename T> Yvar& push_back(const std::vector<T>& items) {
        static_assert(YvarDetail::isSafeValue<T>(), "Yvar::push_back: 不支持的 vector 元素类型");
        ensureArray();
        value.push_back(fromVector(items).value);
        return *this;
    }

    // ── 序列化: 唯一出口 ──────────────────────────────────────────────
    /** @brief 非法 UTF-8 会被替换成 U+FFFD，不会因为元数据里的脏字节抛异常 */
    std::string dump() const;

    // ── 可选链读取(不抛异常) ──────────────────────────────────────────
    Yvar read(std::string_view key) const;
    Yvar read(int index) const;

    /** @brief 需要区分"取到了没有"时用，取不到返回 nullopt */
    std::optional<Yvar> find(std::string_view key) const;
    std::optional<Yvar> find(int index) const;

    bool has(std::string_view key) const;

    // ── 终结取值: 宽松(空值/类型不符 -> 安全默认值) ───────────────────
    /** @brief string 取原文；数字/bool 取 JSON 文本；object/array 取 JSON 文本；空值取 "" */
    std::string toString() const;
    /** @brief 只收集字符串元素，其它类型跳过；非数组返回空 */
    std::vector<std::string> toStringArray() const;
    double toDouble() const;
    int64_t toInt64() const;
    int toInt() const;
    /** @brief bool 取原值；数字取"非 0"；字符串 "true"/"1" 视为 true */
    bool toBool() const;

    template <typename T> std::vector<T> toVector() const {
        static_assert(
            YvarDetail::isSupportedTerminal<T>(),
            "Yvar::toVector<T>: 只支持 bool / 整数 / 浮点 / std::string"
        );
        std::vector<T> vec;
        if (!value.is_array()) return vec;
        vec.reserve(value.size());
        for (const auto& item : value) {
            if constexpr (std::is_same_v<T, bool>) {
                if (item.is_boolean()) vec.push_back(item.get<bool>());
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (item.is_string()) vec.push_back(item.get<std::string>());
            } else if constexpr (std::is_floating_point_v<T>) {
                if (item.is_number()) vec.push_back(static_cast<T>(item.get<double>()));
            } else {
                if (item.is_number()) vec.push_back(static_cast<T>(itemToInt64(item)));
            }
        }
        return vec;
    }

    // ── 终结取值: 严格(缺失/类型不符 -> 抛 TypeError) ─────────────────
    /** @brief 链条末尾的严格取值，和 nlohmann 的 get<T>() 一致；用在必需字段上 */
    template <typename T> T get() const {
        static_assert(
            YvarDetail::isSupportedTerminal<T>(),
            "Yvar::get<T>: 只支持 bool / 整数 / 浮点 / std::string / std::vector<std::string>"
        );
        if (!typeMatches<T>()) throwTypeError(expectedName<T>());

        if constexpr (std::is_same_v<T, std::string>) {
            return value.get<std::string>();
        } else if constexpr (std::is_same_v<T, bool>) {
            return value.get<bool>();
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            return toStringArray();
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(value.get<double>());
        } else {
            return static_cast<T>(itemToInt64(value));
        }
    }

    /** @brief 末尾一条 if 的形态: 取不到就是 nullopt */
    template <typename T> std::optional<T> getOpt() const {
        if (!typeMatches<T>()) return std::nullopt;
        return get<T>();
    }

    // 旧名字，保持既有调用点可用
    std::optional<std::string> toOptStr() const { return getOpt<std::string>(); }
    std::optional<double> toOptDouble() const { return getOpt<double>(); }
    std::optional<int64_t> toOptInt64() const { return getOpt<int64_t>(); }
    std::optional<bool> toOptBool() const { return getOpt<bool>(); }

    // ── 状态 ──────────────────────────────────────────────────────────
    /** @brief 空值(键缺失/下标越界/解析失败)；可选链遇到它不会报错 */
    bool isVoid() const { return value.is_null(); }
    bool isObject() const { return value.is_object(); }
    bool isArray() const { return value.is_array(); }
    bool isString() const { return value.is_string(); }
    /** @brief 元素个数；空值/标量返回 0，所以 for (i < v.size()) 天然安全 */
    int size() const;

    // 需要原始 json 时显式取
    // nlohmann::json& raw() { return value; }
    // const nlohmann::json& raw() const { return value; }
};
