cmake_minimum_required(VERSION 3.22.0)
project(kdadk_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置路径
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(KDADK_LIB_DIR ${PROJECT_ROOT}/lib)
set(KDADK_INCLUDE_DIR ${PROJECT_ROOT}/include)

# ============= ONNX Runtime 配置 =============
set(ONNXRUNTIME_ROOT "$ENV{ONNXRUNTIME_HOME}" CACHE PATH "Root directory of ONNX Runtime installation")
if(NOT ONNXRUNTIME_ROOT)
    set(ONNXRUNTIME_ROOT "${PROJECT_SOURCE_DIR}/util/onnxruntime-linux-aarch64-1.22.0")
endif()

# 查找 ONNX Runtime
find_library(ONNXRUNTIME_LIB
    NAMES onnxruntime
    PATHS ${ONNXRUNTIME_ROOT}
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
    REQUIRED
)

get_filename_component(ONNXRUNTIME_LIB_DIR ${ONNXRUNTIME_LIB} DIRECTORY)
message(STATUS "Found ONNX Runtime library: ${ONNXRUNTIME_LIB}")
message(STATUS "ONNX Runtime library directory: ${ONNXRUNTIME_LIB_DIR}")

# ============= yaml-cpp 配置 =============
find_library(YAML_CPP_LIB
    NAMES yaml-cpp
    PATHS /usr/local/lib64 /usr/lib /usr/lib64
    REQUIRED
)

get_filename_component(YAML_CPP_LIB_DIR ${YAML_CPP_LIB} DIRECTORY)
message(STATUS "Found yaml-cpp library: ${YAML_CPP_LIB}")
message(STATUS "yaml-cpp library directory: ${YAML_CPP_LIB_DIR}")

# 查找 libpcap
find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

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
    kdadk_demo
    main.cpp
    kdadk_file_writer.cpp
    online_inference.cpp
    utils.cpp
)

# 链接库
target_link_libraries(kdadk_demo
    ${KDADK_FEATURE_LIB}
    ${KDADK_INFERENCE_LIB}
    ${PCAP_LIBRARIES}
    ${ONNXRUNTIME_LIB}
    ${YAML_CPP_LIB}
    pthread
    dl
)

# 设置 RPATH
set_target_properties(kdadk_demo PROPERTIES
    BUILD_RPATH "${KDADK_LIB_DIR}"
    INSTALL_RPATH "${KDADK_LIB_DIR}"
)