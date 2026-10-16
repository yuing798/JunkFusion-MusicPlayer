#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
class ArtistsManage {

private:
    CenterDbManage& dbManage;

public:
    ArtistsManage(CenterDbManage& dbManage) : dbManage(dbManage) {}
};