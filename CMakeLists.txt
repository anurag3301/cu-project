cmake_minimum_required(VERSION 3.16)
project(checkers_raylib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(checkers
  src/main.cpp
  src/game/Board.cpp
)

target_link_libraries(checkers PRIVATE raylib)
target_include_directories(checkers PRIVATE src)

if(UNIX AND NOT APPLE)
  target_link_libraries(checkers PRIVATE m pthread dl rt X11)
endif()
