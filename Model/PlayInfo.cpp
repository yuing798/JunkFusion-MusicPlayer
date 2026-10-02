#include "./PlayInfo.hpp"

juce::var PlayInfo::toJson() {
    juce::var obj{new juce::DynamicObject()};
    auto ptr{obj.getDynamicObject()};
    ptr->setProperty(SongInfoMacro::path, path);
    ptr->setProperty(SongInfoMacro::artists, ConvertUtils::stringArray2ArrayVar(artists));
    ptr->setProperty(SongInfoMacro::duration, duration);
    ptr->setProperty(SongInfoMacro::title, title);
    ptr->setProperty(SongInfoMacro::hash, ConvertUtils::juceStringToVar(hash));
    return obj;
}
PlayInfo PlayInfo::fromJson(Yvar json) {
    PlayInfo info{};
    info.path = json.read(SongInfoMacro::path).toString();
    info.artists = json.read(SongInfoMacro::artists).toStringArray();
    info.duration = json.read(SongInfoMacro::duration).toDouble();
    info.hash = json.read(SongInfoMacro::hash).toString();
    info.title = json.read(SongInfoMacro::title).toString();
    return info;
}