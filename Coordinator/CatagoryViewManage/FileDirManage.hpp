#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
class FileDirManage {
private:
    CenterDbManage& dbManage;

public:
    FileDirManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    Yvar getAllRootDirView();
    Yvar getChileFiles(const juce::File& parentPath);
};