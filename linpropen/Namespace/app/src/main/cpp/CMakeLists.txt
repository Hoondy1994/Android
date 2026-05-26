cmake_minimum_required(VERSION 3.22.1)

project("myapplication")

set(NS_CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../PantanalNamespace/mylibrary3/src/main/cpp)

add_subdirectory(${NS_CPP_DIR}/utils utils)
add_subdirectory(${NS_CPP_DIR}/libpreopen libpreopen)

add_library(${CMAKE_PROJECT_NAME} SHARED
        sandbox_jni.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${NS_CPP_DIR}/libpreopen/include
        ${NS_CPP_DIR}/utils)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        preopen
        utils)
