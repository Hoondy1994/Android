cmake_minimum_required(VERSION 3.22.1)
project("filefuse")

add_library(${CMAKE_PROJECT_NAME} SHARED
        jni/filefuse_jni.cpp
        vfs/vfs.cpp
        backends/local_backend.cpp
        backends/nfs/nfs_rpc.cpp
        backends/nfs/nfs_backend.cpp
        fuse/fuse_adapter.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)
