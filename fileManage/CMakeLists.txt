target_include_directories(JunkFusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusion PRIVATE
    fileUtils.cpp
    songsManage.cpp#数据库管理
    # serial.cpp#序列化和反序列化
)