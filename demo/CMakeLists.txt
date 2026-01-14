cmake_minimum_required(VERSION 3.10)
project(traffic_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置路径
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(KDADK_LIB_DIR ${PROJECT_ROOT}/lib)
set(KDADK_INCLUDE_DIR ${PROJECT_ROOT}/include)

# 查找 libpcap
find_package(PkgConfig REQUIRED)
pkg_ck_modules(PCAP REQUIRED libpcap)

# 查找 KDADK 库
find_library(KDADK_FEATURE_LIB
    NAMES KDADK_feature_extract libKDADK_feature_extract.so
    PATHS ${KDADK_LIB_DIR}
    NO_DEFAULT_PATH
    REQUIRED
)

find_library(KDADK_INFERENCE_LIB
    NAMES KDADK_inference libKDADK_inference.so
    PATHS ${KDADK_LIB_DIR}
    NO_DEFAULT_PATH
    REQUIRED
)

message(STATUS "Found KDADK_feature_extract: ${KDADK_FEATURE_LIB}")
message(STATUS "Found KDADK_inference: ${KDADK_INFERENCE_LIB}")

# 包含目录
include_directories(
    ${KDADK_INCLUDE_DIR}
    ${PCAP_INCLUDE_DIRS}
    ${PROJECT_ROOT}/demo
)

# 可执行文件
add_executable(
    traffic_demo
    main.cpp
    kdadk_file_writer.cpp
    online_inference.cpp
    utils.cpp
)

# 链接库
target_link_libraries(traffic_demo
    ${KDADK_FEATURE_LIB}
    ${KDADK_INFERENCE_LIB}
    ${PCAP_LIBRARIES}
    pthread
    dl
)

# 设置 RPATH
set_target_properties(traffic_demo PROPERTIES
    BUILD_RPATH "${KDADK_LIB_DIR}"
    INSTALL_RPATH "${KDADK_LIB_DIR}"
)