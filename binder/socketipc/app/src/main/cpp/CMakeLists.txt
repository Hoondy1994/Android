cmake_minimum_required(VERSION 3.22.1)
project("myapplication")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${CMAKE_PROJECT_NAME} SHARED
        src/types.cpp
        src/custom_codec.cpp
        src/parcel_codec.cpp
        src/socket_protocol.cpp
        src/socket_server.cpp
        src/socket_client.cpp
        src/benchmark.cpp
        src/jni_util.cpp
        jni/jni_bridge.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)
