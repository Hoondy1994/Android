cmake_minimum_required(VERSION 3.22.1)
project("audiotest")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wno-unused-parameter)

set(AUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/audio)

add_library(${CMAKE_PROJECT_NAME} SHARED
    native-lib.cpp
    ${AUDIO_DIR}/audio_recorder.cpp
    ${AUDIO_DIR}/audio_player.cpp
    ${AUDIO_DIR}/audio_engine.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    aaudio
    android
    log
)
