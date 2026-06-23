target_include_directories(JunkFusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusion PRIVATE
    LeftColumn.cpp
    mainPage.cpp
    uiUtils.cpp
)

add_subdirectory(FontAbout)
add_subdirectory(Pages)
add_subdirectory(diyComponent)