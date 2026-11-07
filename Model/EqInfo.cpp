#include "./EqInfo.hpp"
#include "Utils/Yvar.hpp"
#include <cstddef>
#include <optional>
#include <string_view>

const char* EqBandParam::typeName() const {
    // 新增类型时，这里和 typeFromName 两处都要改（故意不建映射表，避免再引入宏/表）
    switch (type) {
        case BandType::Bell: return "bell";
        case BandType::LowPass: return "lowPass";
        case BandType::HighPass: return "highPass";
        case BandType::Notch: return "notch";
        case BandType::BandPass: return "bandPass";
        case BandType::AllPass: return "allPass";
        case BandType::LowShelf: return "lowShelf";
        case BandType::HighShelf: return "highShelf";
        case BandType::Tile: return "tile";
        case BandType::FlatTile: return "flatTile";
    }
    return "bell"; // 兜底，防止以后加类型忘了改这里
}

std::optional<BandType> EqBandParam::typeFromName(std::string_view name) {
    if (name == "bell") return BandType::Bell;
    if (name == "lowPass") return BandType::LowPass;
    if (name == "highPass") return BandType::HighPass;
    if (name == "notch") return BandType::Notch;
    if (name == "bandPass") return BandType::BandPass;
    if (name == "allPass") return BandType::AllPass;
    if (name == "lowShelf") return BandType::LowShelf;
    if (name == "highShelf") return BandType::HighShelf;
    if (name == "tile") return BandType::Tile;
    if (name == "flatTile") return BandType::FlatTile;
    return std::nullopt; // 认不出来就让调用方保持原类型，不要静默改行为
}

// ── 编码 ────────────────────────────────────────────────────────────────
Yvar EqWire::encodeBand(const EqBandParam& band) {
    Yvar obj = Yvar::object();
    obj.set("idx", band.idx);
    obj.set("on", band.enabled);
    obj.set("type", band.typeName());
    obj.set("freqHz", band.freqHz);
    obj.set("gainDb", band.gainDb);
    obj.set("q", band.q);
    obj.set("shelfSlope", band.shelfSlope);
    obj.set("cutSlope", band.cutSlopeDbPerOct);
    return obj;
}

Yvar EqWire::encodeMaster(bool enabled, float masterGainDb) {
    Yvar obj = Yvar::object();
    obj.set("on", enabled);
    obj.set("gainDb", masterGainDb);
    return obj;
}

Yvar EqWire::encodeAll(const EqAllParam& all) {
    Yvar obj = Yvar::object();
    obj.set("on", all.enabled);
    obj.set("gainDb", all.masterGainDb);

    Yvar bands = Yvar::array();
    for (int i = 0; i < EqBandParam::kMaxBands; ++i) {
        EqBandParam band = all.bands[static_cast<size_t>(i)];
        band.idx = i; // 数组下标即序号，写进报文方便排查
        bands.push(encodeBand(band));
    }
    obj.set("bands", bands);
    return obj;
}

// ── 解码：全部走 Yvar 可选链，缺字段自动落回默认值，不抛异常 ────────────
EqBandParam EqWire::decodeBand(const Yvar& body) {
    EqBandParam band{};
    band.idx = body.read("idx").toInt();
    band.enabled = body.read("on").toBool();

    if (auto typeName = body.read("type").toOptStr()) {
        if (auto type = EqBandParam::typeFromName(*typeName)) band.type = *type;
    }

    band.freqHz = static_cast<float>(body.read("freqHz").toOptDouble().value_or(1000.0));
    band.gainDb = static_cast<float>(body.read("gainDb").toOptDouble().value_or(0.0));
    band.q = static_cast<float>(body.read("q").toOptDouble().value_or(0.70710678));
    band.shelfSlope = static_cast<float>(body.read("shelfSlope").toOptDouble().value_or(1.0));

    const int cutSlope = body.read("cutSlope").toInt();
    band.cutSlopeDbPerOct = cutSlope > 0 ? cutSlope : 12;
    return band;
}

EqAllParam EqWire::decodeAll(const Yvar& body) {
    EqAllParam all{};

    // "on" 缺省时应该是 true（预设里没写 = 开着），所以不能直接用 toBool 的 false 默认
    all.enabled = body.has("on") ? body.read("on").toBool() : true;
    all.masterGainDb = static_cast<float>(body.read("gainDb").toOptDouble().value_or(0.0));

    const Yvar bands = body.read("bands");
    for (int i = 0; i < EqBandParam::kMaxBands && i < bands.size(); ++i) {
        all.bands[static_cast<size_t>(i)] = decodeBand(bands.read(i));
        all.bands[static_cast<size_t>(i)].idx = i;
    }
    return all;
}

std::optional<EqWire::Message> EqWire::parse(const Yvar& envelope) {
    if (!envelope.isObject()) return std::nullopt;

    Message msg{};
    msg.rev = static_cast<uint32_t>(envelope.read("rev").toInt64());

    if (envelope.has("eqBand")) {
        msg.kind = Kind::band;
        msg.band = decodeBand(envelope.read("eqBand"));
        return msg;
    }
    if (envelope.has("eqMaster")) {
        msg.kind = Kind::master;
        const Yvar body = envelope.read("eqMaster");
        msg.masterEnabled = body.has("on") ? body.read("on").toBool() : true;
        msg.masterGainDb = static_cast<float>(body.read("gainDb").toOptDouble().value_or(0.0));
        return msg;
    }
    if (envelope.has("eqAll")) {
        msg.kind = Kind::all;
        msg.all = decodeAll(envelope.read("eqAll"));
        msg.masterEnabled = msg.all.enabled;
        msg.masterGainDb = msg.all.masterGainDb;
        return msg;
    }
    return std::nullopt;
}

// ── 采样率上报（音频进程 -> Coordinator，画频响曲线用） ──────────────────
Yvar EqWire::encodeSampleRate(double sampleRate) {
    Yvar obj = Yvar::object();
    obj.set("audioSampleRate", sampleRate);
    return obj;
}

std::optional<double> EqWire::decodeSampleRate(const Yvar& envelope) {
    if (!envelope.isObject() || !envelope.has("audioSampleRate")) return std::nullopt;

    const auto value = envelope.read("audioSampleRate").toOptDouble();
    if (!value.has_value() || !(*value > 0.0)) return std::nullopt;
    return value;
}
