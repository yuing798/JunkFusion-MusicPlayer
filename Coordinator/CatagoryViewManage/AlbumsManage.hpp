#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <cstdint>
#include <optional>
namespace AlbumsManage {
    juce::Array<juce::var> getAllAlbumsView(CenterDbManage* dbManage);

    std::optional<int64_t> checkAlbumExist(
        CenterDbManage* dbManage,
        juce::String title,
        juce::StringArray artists,
        juce::String hash
    );

    std::optional<int64_t> insertAlbum(
        CenterDbManage* dbManage,
        juce::String title,
        juce::StringArray artists,
        juce::String hash
    );
}; // namespace AlbumsManage