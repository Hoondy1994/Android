# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("PantanalNamespace")

add_library( # Sets the name of the library.
        ule_file1
        # Sets the library as a shared library.
        SHARED
        FilePlugin.cpp)

target_link_libraries( # Specifies the target library.
        ule_file1
        ulenapi  ###### Add ule napi for link ######
        utils
        namespace)

add_subdirectory(ule-napi)
add_subdirectory(libnamespace)
add_subdirectory(libpreopen)
add_subdirectory(tests)
ADD_SUBDIRECTORY(utils)

# JNI demo for app: libpreopen path permission control (no ULE/NAPI runtime required)
add_library(preopen_demo SHARED preopen_jni.cpp)
target_include_directories(preopen_demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libpreopen/include)
target_link_libraries(preopen_demo preopen utils log)