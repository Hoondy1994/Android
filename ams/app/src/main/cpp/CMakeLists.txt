cmake_minimum_required(VERSION 3.22.1)
project("amstest")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Extra warnings
add_compile_options(-Wall -Wextra -Wno-unused-parameter)

set(AMS_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ams)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        ${AMS_SRC_DIR}/activity_record.cpp
        ${AMS_SRC_DIR}/process_record.cpp
        ${AMS_SRC_DIR}/task_record.cpp
        ${AMS_SRC_DIR}/activity_stack.cpp
        ${AMS_SRC_DIR}/activity_manager_service.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
)
