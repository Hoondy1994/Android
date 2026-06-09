cmake_minimum_required(VERSION 3.22.1)
project("surfacetest")

add_library(${CMAKE_PROJECT_NAME} SHARED
        jni_bridge.cpp
        egl_renderer.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        EGL
        GLESv3)
