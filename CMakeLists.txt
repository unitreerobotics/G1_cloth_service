cmake_minimum_required(VERSION 3.10.0)
project(JqReceiver VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put shared libs next to the executable for easier local runs (esp. Windows).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    add_compile_options("/utf-8")
else()
    add_compile_options(-Wall -Wextra)
endif()

find_package(Threads REQUIRED)

# ===== unitree_sdk2 (DDS) =====
# Select the prebuilt library matching the target compiler (host toolchain
# does not set CMAKE_SYSTEM_PROCESSOR for the cross build).
if(CMAKE_CXX_COMPILER MATCHES "aarch64")
    set(UNITREE_SDK_ARCH aarch64)
else()
    set(UNITREE_SDK_ARCH x86_64)
endif()
message(STATUS "unitree_sdk2 arch: ${UNITREE_SDK_ARCH}")

set(UNITREE_SDK2_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../G1-D/move_service/3rd/unitree_sdk2)
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

set(JQ_RECEIVER_LIB_SOURCES
    src/JqProtocol.cpp
    src/JqReceiver.cpp
    src/SerialPort.cpp
    src/JqDds.cpp
)

# ===== Shared library for upper-layer consumers =====
add_library(jq_receiver SHARED ${JQ_RECEIVER_LIB_SOURCES})

target_include_directories(jq_receiver
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${UNITREE_SDK2_INCLUDE_DIRS}
)

target_compile_definitions(jq_receiver
    PRIVATE JQ_RECEIVER_SHARED JQ_RECEIVER_BUILDING
    INTERFACE JQ_RECEIVER_SHARED
)

target_link_libraries(jq_receiver PUBLIC Threads::Threads ${UNITREE_SDK2_LIBS})

if(WIN32)
    target_link_libraries(jq_receiver PUBLIC ws2_32)
endif()

set_target_properties(jq_receiver PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    OUTPUT_NAME jq_receiver
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)

# ===== Demo executable (links against the shared library) =====
add_executable(JqReceiver src/main.cpp)
target_include_directories(JqReceiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${UNITREE_SDK2_INCLUDE_DIRS}
)
target_link_libraries(JqReceiver PRIVATE jq_receiver)

# ===== DDS subscriber test tool =====
add_executable(JqDdsTest test/JqDdsTest.cpp)
target_include_directories(JqDdsTest PRIVATE ${UNITREE_SDK2_INCLUDE_DIRS})
target_link_libraries(JqDdsTest PRIVATE ${UNITREE_SDK2_LIBS} Threads::Threads)

if(MINGW)
    target_link_libraries(JqReceiver PRIVATE -static-libgcc -static-libstdc++)
endif()

# ===== Install rules (headers + shared library) =====
include(GNUInstallDirs)

install(TARGETS jq_receiver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    include/jq_receiver_export.h
    include/SerialPort.h
    include/JqProtocol.h
    include/JqReceiver.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jq_receiver
)
