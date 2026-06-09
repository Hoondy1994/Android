cmake_minimum_required(VERSION 3.22.1)
project("myapplication")

add_library(${CMAKE_PROJECT_NAME} SHARED
        hal_jni.cpp
        hal/hal_registry.cpp
        hal/sensor_hal_native.cpp
        hal/gpio_hal_native.cpp
        hal/power_hal_native.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log)
