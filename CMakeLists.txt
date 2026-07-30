cmake_minimum_required(VERSION 3.16)
project(music_player VERSION 0.1 LANGUAGES CXX C) # miniaudio는 C 파일로 동작하므로 C 언어 선언 필요!

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)

# miniaudio 헤더 파일 경로 포함
include_directories(libs/miniaudio.h)

add_executable(music_player
        main.cpp
        separated_class/MusicPlayerWindow.cpp
        separated_class/AudioEngine.cpp
        separated_class/MarqueeLabel.cpp
)

# 리눅스 사운드 및 스레드 필수 시스템 라이브러리 (m, dl, pthread)
target_link_libraries(music_player PRIVATE
        Qt6::Widgets
        ${TAGLIB_LIBRARIES}
        m dl pthread
)

target_include_directories(music_player PRIVATE ${TAGLIB_INCLUDE_DIRS})