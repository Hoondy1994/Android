set(LIBRARY_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

add_library(${PROJECT_NAME} STATIC
		libpreopen.cpp
		po_err.cpp
		po_libc_wrappers.cpp
		po_map.cpp
		libseqmap.cpp)

target_include_directories(
		${PROJECT_NAME}
		PUBLIC
		${PROJECT_SOURCE_DIR}/include)

target_link_libraries(
		${PROJECT_NAME}
		utils)
