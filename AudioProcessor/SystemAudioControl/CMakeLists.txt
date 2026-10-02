# SystemAudioControl —— 系统级音频服务控制（通知栏/控制中心显示 + 耳机/键盘媒体按键）
cmake_minimum_required(VERSION 3.22)

project(SystemAudioControl VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
# endif()

# 先声明 target
add_library(SystemAudioControl STATIC)

target_include_directories(SystemAudioControl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if (WIN32)
    target_sources(SystemAudioControl PRIVATE WindowsSystemAudioControl.cpp)
    # TODO(连接): 若链接报缺失 WinRT 符号请替换为 windowsapp.lib
    target_link_libraries(SystemAudioControl PUBLIC onecoreuap)
elseif (APPLE)
    # 更规范做法：把 .cpp 改名为 .mm，然后删掉下面这行 set_source_files_properties
    # set_source_files_properties(MacosSystemAudioControl.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
    target_sources(SystemAudioControl PRIVATE MacosSystemAudioControl.mm)
    target_link_libraries(SystemAudioControl PUBLIC
        "-framework Foundation"
        "-framework MediaPlayer"
        "-framework AVFoundation"
    )
elseif (UNIX AND NOT APPLE)
    target_sources(SystemAudioControl PRIVATE LinuxSystemAudioControl.cpp)
    # 依赖 libdbus-1，目标机需安装 libdbus-1-dev（Debian/Ubuntu）或 dbus-devel（Fedora）
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DBUS REQUIRED dbus-1)
    target_include_directories(SystemAudioControl PRIVATE ${DBUS_INCLUDE_DIRS})
    target_link_directories(SystemAudioControl PRIVATE ${DBUS_LIBRARY_DIRS})
    target_link_libraries(SystemAudioControl PUBLIC ${DBUS_LIBRARIES})
endif ()