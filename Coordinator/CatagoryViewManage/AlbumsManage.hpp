#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <cstdint>
#include <optional>
class AlbumsManage {
private:
    CenterDbManage& dbManage;

public:
    AlbumsManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    juce::Array<juce::var> getAllAlbumsView();

    std::optional<int64_t> checkAlbumExist(
        juce::String albumTitle,
        juce::StringArray artists,
        std::optional<juce::String> hash
    );

    std::optional<int64_t> insertAlbum(
        juce::String albumTitle,
        juce::StringArray artists,
        std::optional<juce::String> hash
    );
    std::optional<int64_t> getAlbumId(int64_t songId);
}; // namespace AlbumsManage