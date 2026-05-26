project("ulenapi")

set(ULE_NAPI_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

#add_library(ulenapi INTERFACE IMPORTED)
#target_link_libraries(ulenapi
#        INTERFACE
#        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/${ANDROID_ABI}/libule_napi.so>)
#
#target_include_directories(ulenapi
#        INTERFACE
#        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

add_library(${PROJECT_NAME} SHARED IMPORTED GLOBAL)
set_target_properties(${PROJECT_NAME} PROPERTIES
        IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/src/${ANDROID_ABI}/libule_napi.so
        INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/include")