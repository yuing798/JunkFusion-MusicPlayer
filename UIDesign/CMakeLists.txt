target_include_directories(JunkFusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusion PRIVATE
    UISet.cpp
    LeftSelectedComponent.cpp
)

add_subdirectory(FontAbout)