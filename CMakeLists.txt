cmake_minimum_required(VERSION 3.16)
project(plc-learn VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找依赖包
find_package(PkgConfig REQUIRED)

# 查找SQLite3
find_package(SQLite3)
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 not found via find_package, using system library")
    set(SQLite3_LIBRARIES sqlite3)
    set(SQLite3_INCLUDE_DIRS "")
endif()

# 查找jsoncpp
pkg_check_modules(JSONCPP jsoncpp)
if(NOT JSONCPP_FOUND)
    message(STATUS "jsoncpp not found via pkg-config, will use system library")
endif()

# 包含目录
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/cpp/*.cpp"
    "src/modbus/*.cpp"
    "src/socket/*.cpp"
    "src/database/*.cpp"  # 重新启用
    "src/analysis/*.cpp"
)

# 头文件
file(GLOB_RECURSE HEADERS 
    "include/*.h"
    "src/*.h"
)

# 创建可执行文件
add_executable(plc-simulator ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(plc-simulator 
    sqlite3  # 重新启用
    pthread
)

# 包含头文件目录
target_include_directories(plc-simulator PRIVATE 
    ${SQLite3_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

# 设置输出目录
set_target_properties(plc-simulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build
)

# 安装规则
install(TARGETS plc-simulator DESTINATION bin)
install(FILES ${HEADERS} DESTINATION include/plc-learn)
