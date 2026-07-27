find_package(Qt6 6.8 REQUIRED COMPONENTS 
    Core 
    Quick
    Qml
)
qt_standard_project_setup()
qt_add_executable(JunkFusionUI)

target_sources(JunkFusionUI PRIVATE
    main.cpp)

# add_custom_command(TARGET JunkFusionUI POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E echo "[Paths]" > "$<TARGET_FILE_DIR:JunkFusionUI>/qt.conf"
#     COMMAND ${CMAKE_COMMAND} -E echo "Plugins = ../build/debug/vcpkg_installed/x64-windows/$<CONFIG>/Qt6/plugins" >> "$<TARGET_FILE_DIR:JunkFusionUI>/qt.conf"
#     COMMENT "Generating qt.conf → vcpkg plugin discovery"
# )

target_link_libraries(JunkFusionUI
    PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
)
