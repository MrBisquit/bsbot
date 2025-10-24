cmake_minimum_required(VERSION 3.15)

project(bsbot C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(raylib)

add_executable(bsbot src/main.c)
target_link_libraries(bsbot raylib)

set_target_properties(bsbot PROPERTIES LINK_SEARCH_START_STATIC ON)
set_target_properties(bsbot PROPERTIES LINK_SEARCH_END_STATIC ON)

set(RAYLIB_VERBOSE OFF)