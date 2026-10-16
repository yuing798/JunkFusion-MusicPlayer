#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
class FileDirManage {
private:
    CenterDbManage& dbManage;

public:
    FileDirManage(CenterDbManage& dbManage) : dbManage(dbManage) {}
};