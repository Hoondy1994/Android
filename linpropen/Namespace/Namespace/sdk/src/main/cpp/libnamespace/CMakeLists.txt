cmake_minimum_required(VERSION 3.18.1)

project("namespace")

add_library( # Sets the name of the library.
        ${PROJECT_NAME}
        # Sets the library as a shared library.
        SHARED
        NamespaceLibc.cpp
        Namespace.cpp
        uri.cpp)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

target_link_libraries( # Specifies the target library.
        ${PROJECT_NAME}
        utils
        # napi
        ulenapi
        # libpreopen
        preopen)