cmake_minimum_required(VERSION 3.0.2)

project(preopen)

# Always build with all warnings.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes")

add_subdirectory(lib)