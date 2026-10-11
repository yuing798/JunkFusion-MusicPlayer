#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
class PlayListManage {
private:
    CenterDbManage& dbManage;

public:
    PlayListManage(CenterDbManage& dbManage) : dbManage(dbManage) {}
};