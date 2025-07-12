cmake_minimum_required(VERSION 3.15)
project(snake C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)

FetchContent_Declare(
    flecs
    GIT_REPOSITORY "https://github.com/SanderMertens/flecs.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib)
FetchContent_MakeAvailable(flecs)

file(GLOB_RECURSE SOURCE_FILES
    src/*.c
    src/*.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    flecs::flecs_static
)