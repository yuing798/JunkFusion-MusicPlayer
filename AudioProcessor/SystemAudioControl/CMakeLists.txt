# SystemAudioControl —— 系统级音频服务控制（通知栏/控制中心显示 + 耳机/键盘媒体按键）
# 对外暴露一个静态库，父级 CMakeLists 通过 add_subdirectory 引入后链接即可。
add_library(SystemAudioControl STATIC)

target_include_directories(SystemAudioControl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(SystemAudioControl
    PUBLIC
        juce::juce_core
)

if (WIN32)
    target_sources(SystemAudioControl PRIVATE WindowsSystemAudioControl.cpp)
    # TODO(连接): SMTC/WinRT 符号来自 windowsapp.lib，若链接报缺失 WinRT 符号请替换为 onecoreuap.lib
    target_link_libraries(SystemAudioControl PUBLIC windowsapp)
elseif (APPLE)
    # TODO(连接): 本实现使用 Objective-C++，这里强制 .cpp 按 ObjC++ 编译；更规范做法是把
    #             MacosSystemAudioControl.cpp 改名为 MacosSystemAudioControl.mm 后删除此属性
    set_source_files_properties(MacosSystemAudioControl.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    target_sources(SystemAudioControl PRIVATE MacosSystemAudioControl.cpp)
    target_link_libraries(SystemAudioControl PUBLIC
        "-framework Foundation"
        "-framework MediaPlayer"
        "-framework AVFoundation"
    )
elseif (UNIX AND NOT APPLE)
    target_sources(SystemAudioControl PRIVATE LinuxSystemAudioControl.cpp)
    # TODO(连接): 依赖 libdbus-1，目标机需安装 libdbus-1-dev（Debian/Ubuntu）或 dbus-devel（Fedora）
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    target_include_directories(SystemAudioControl PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_directories(SystemAudioControl PRIVATE ${DBUS_LIBRARY_DIRS})
    target_link_libraries(SystemAudioControl PUBLIC ${DBUS_LIBRARIES})
endif ()
