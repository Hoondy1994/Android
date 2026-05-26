cmake_minimum_required(VERSION 3.18.1)

project("utils")

#find_library( # Sets the name of the path variable.
#        log-lib
#        # Specifies the name of the NDK library that
#        # you want CMake to locate.
#        log)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
        INTERFACE
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} INTERFACE log)
endif()

#target_link_libraries( # Specifies the target library.
#        ${PROJECT_NAME}
#        # Links the target library to the log library
#        # included in the NDK.
#        ${log-lib})