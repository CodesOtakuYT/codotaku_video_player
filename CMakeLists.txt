cmake_minimum_required(VERSION 4.1)
project(codotaku_video_player C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(SDL3 3.4 REQUIRED)
find_package(PkgConfig)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil)

add_executable(codotaku_video_player src/main.c)
target_link_libraries(codotaku_video_player PRIVATE SDL3::SDL3 PkgConfig::FFMPEG)