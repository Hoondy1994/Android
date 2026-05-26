# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.18.1)

# Declares and names the project.

project("PantanalNamespace")

add_library( # Sets the name of the library.
        ule_file
        # Sets the library as a shared library.
        SHARED
        FilePlugin.cpp)

target_link_libraries( # Specifies the target library.
        ule_file
        ulenapi  ###### Add ule napi for link ######
        utils
        namespace)

add_subdirectory(ule-napi)
add_subdirectory(libnamespace)
add_subdirectory(libpreopen)
add_subdirectory(tests)
ADD_SUBDIRECTORY(utils)