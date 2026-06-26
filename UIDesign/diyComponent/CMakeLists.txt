target_include_directories(JunkFusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(JunkFusion PRIVATE
    buttons.cpp
    lookandfeel.cpp
    otherComponent.cpp
    YComboBox.cpp
    popupWindow.cpp
)
add_subdirectory(cell)