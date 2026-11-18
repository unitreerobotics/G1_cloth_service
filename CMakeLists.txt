cmake_minimum_required(VERSION 3.10.0)
project(cloth_service VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

# ===== unitree_sdk2 (DDS) =====
# 根据当前主机架构选择对应的预编译库
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(UNITREE_SDK_ARCH aarch64)
else()
    set(UNITREE_SDK_ARCH x86_64)
endif()
message(STATUS "unitree_sdk2 arch: ${UNITREE_SDK_ARCH}")

# 本地编译：使用本仓库自带的 SDK（3rd/unitree_sdk2）
set(UNITREE_SDK2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd/unitree_sdk2)
if(NOT EXISTS ${UNITREE_SDK2_DIR}/include)
    message(FATAL_ERROR "local unitree_sdk2 not found: ${UNITREE_SDK2_DIR}")
endif()
set(UNITREE_SDK2_INCLUDE_DIRS
    ${UNITREE_SDK2_DIR}/include
    ${UNITREE_SDK2_DIR}/thirdparty/include
    ${UNITREE_SDK2_DIR}/thirdparty/include/ddscxx
    ${UNITREE_SDK2_DIR}/thirdparty/include/iceoryx/v2.0.2
)
set(UNITREE_SDK2_LIBS
    ${UNITREE_SDK2_DIR}/lib/${UNITREE_SDK_ARCH}/libunitree_sdk2.a
    ${UNITREE_SDK2_DIR}/thirdparty/lib/${UNITREE_SDK_ARCH}/libddscxx.so
    ${UNITREE_SDK2_DIR}/thirdparty/lib/${UNITREE_SDK_ARCH}/libddsc.so
    ${UNITREE_SDK2_DIR}/thirdparty/lib/${UNITREE_SDK_ARCH}/libssl.so
    ${UNITREE_SDK2_DIR}/thirdparty/lib/${UNITREE_SDK_ARCH}/libcrypto.so
)
link_directories(${UNITREE_SDK2_DIR}/thirdparty/lib/${UNITREE_SDK_ARCH})

# ENABLE_DEBUG_MODE 控制 bp_logger 是否输出到终端（Debug 开 / 其他关）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(ENABLE_DEBUG_MODE=1)
else()
    add_compile_definitions(ENABLE_DEBUG_MODE=0)
endif()

# ===== Main executable (all sources compiled directly) =====
add_executable(cloth_service
    src/main.cpp
    src/JqProtocol.cpp
    src/cloth_service.cpp
    src/SerialPort.cpp
    src/JqDds.cpp
    utils/bp_logger.cpp
)

target_include_directories(cloth_service PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${UNITREE_SDK2_INCLUDE_DIRS}
)

target_link_libraries(cloth_service PRIVATE Threads::Threads ${UNITREE_SDK2_LIBS})

# ===== DDS subscriber test tool =====
add_executable(JqDdsTest test/JqDdsTest.cpp)
target_include_directories(JqDdsTest PRIVATE ${UNITREE_SDK2_INCLUDE_DIRS})
target_link_libraries(JqDdsTest PRIVATE ${UNITREE_SDK2_LIBS} Threads::Threads)

# ===== Install rule =====
install(TARGETS cloth_service RUNTIME DESTINATION bin)
