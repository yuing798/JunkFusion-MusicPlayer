# cmake/DeployBinaries.cmake
# 用法：cmake -D SRC_DIR=... -D DST_DIR=... -P DeployBinaries.cmake

message(STATUS "正在部署运行时库: ${SRC_DIR} -> ${DST_DIR}")

# 1. 确保目标文件夹存在
file(MAKE_DIRECTORY "${DST_DIR}")

# 2. 只扫描 .dll 和 .exe 文件（自动屏蔽了 .ilk, .pdb, .lib, .exp 等垃圾文件）
file(GLOB FILES_TO_COPY 
     "${SRC_DIR}/*.dll" 
     "${SRC_DIR}/*.exe"
)

foreach(FILE ${FILES_TO_COPY})
    get_filename_component(FILENAME "${FILE}" NAME)
    message(STATUS "  复制: ${FILENAME}")
    
    # 使用 copy 命令覆盖目标文件（如果你希望仅在更新时复制，可用 copy_if_different）
    file(COPY "${FILE}" DESTINATION "${DST_DIR}")
endforeach()

message(STATUS "部署完成！")