#include "./FileDirManage.hpp"
#include "Macro/FileMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Exception.h>
#include <filesystem>
#include <spdlog/spdlog.h>

Yvar FileDirManage::getAllRootDirView() {
    Yvar folders = Yvar::array();
    try {
        SQLite::Statement selectSql{dbManage.readDb(), "SELECT rootPath FROM songFolders"};

        while (selectSql.executeStep()) {
            folders.push_back(selectSql.getColumn("rootPath").getString());
        }
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "FileDirManage::getAllRootDirWiew throw db error:{}",
            e.what()
        );
    }
    return folders;
}
Yvar FileDirManage::getChileFiles(const juce::File& parentPath) {
    auto obj = Yvar::object();
    auto dirArr = Yvar::array();
    auto fileArr = Yvar::array();
    auto children = parentPath.findChildFiles(juce::File::findFilesAndDirectories, false);
    for (const auto& child : children) {
        if (child.isDirectory()) {
            dirArr.push_back(child.getFullPathName().toStdString());
        } else {
            fileArr.push_back(child.getFullPathName().toStdString());
        }
    }
    obj.set(FileMacro::fileArr, fileArr);
    obj.set(FileMacro::dirArr, dirArr);
    return obj;
}