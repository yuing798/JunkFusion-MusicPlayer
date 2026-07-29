target_include_directories(JunkFusionBackend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusionBackend PRIVATE
    songsManage.cpp#数据库管理
    # serial.cpp#序列化和反序列化
)