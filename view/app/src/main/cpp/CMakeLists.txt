cmake_minimum_required(VERSION 3.22.1)
project("viewtest")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

set(VIEW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/view)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        ${VIEW_DIR}/particle_system.cpp
        ${VIEW_DIR}/waveform.cpp
        ${VIEW_DIR}/layout_solver.cpp
        ${VIEW_DIR}/view_engine.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
)
