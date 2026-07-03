target_include_directories(JunkFusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusion PRIVATE
    linkCppAndWebview.cpp#这个文件用来链接前端代码和cpp层
)