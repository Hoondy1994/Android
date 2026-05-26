if (${ENABLE_UNIT_TESTS} MATCHES "true")

    # unit tests
    cmake_minimum_required(VERSION 3.18.1)
    message("sddddddddddddddddddddd")
    # Declares and names the project.
    project("unittest")

    set(CMAKE_CXX_STANDARD 17)

    find_package(googletest REQUIRED CONFIG)

    # unit test binary file location
    set(NS_TEST_DIR /data/local/tmp/ns)
    # unit test linked libraries
    set(NS_TEST_LIB_DIR ${NS_TEST_DIR}/libs)
    set(NS_TEST_LIB_DIR /data/local/tmp/ns/lib)
    # adb location
    find_program(ADB NAMES adb PATHS ${ANDROID_SDK_DIR}/platform-tools)

    add_executable(
            ${PROJECT_NAME}
            main.cpp
            libpreopen_tests.cpp
            liburi_test.cpp)
    message("kkkkkkkkkkkkkkkkkkk")
    target_link_libraries( # Specifies the target library.
            ${PROJECT_NAME}
            googletest::gtest
            preopen
            utils
            namespace)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            # mkdir
            COMMAND ${ADB} shell mkdir -p ${NS_TEST_DIR}
            COMMAND ${ADB} shell mkdir -p ${NS_TEST_LIB_DIR}

            # push binary
            COMMAND ${ADB} push $<TARGET_FILE:${PROJECT_NAME}> ${NS_TEST_DIR}/
            # push libraries
            COMMAND ${ADB} push $<TARGET_FILE:namespace> ${NS_TEST_LIB_DIR}/
            COMMAND ${ADB} push $<TARGET_FILE:ulenapi> ${NS_TEST_LIB_DIR}/

            # chmod 777
            COMMAND ${ADB} shell chmod 777 ${NS_TEST_DIR}/${PROJECT_NAME}

            # FIXME don't know how to see output of result, should manual execute this test
            # set env & execute
            # COMMAND ${ADB} shell \"export LD_LIBRARY_PATH=${NS_TEST_LIB_DIR}\; ${NS_TEST_DIR}/${PROJECT_NAME}\"
            )
endif()