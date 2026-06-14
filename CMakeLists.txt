cmake_minimum_required(VERSION 3.20)
project(music_player CXX)

set(CMAKE_CXX_STANDARD 20)

# Qt 자동화 설정
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 1. 필수 라이브러리 탐색 (Threads, TagLib, Qt6)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 2. 🌟 [경로 수정] 리눅스 PC용(linux_x64) BASS 라이브러리 경로 및 위치 찾기
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/include)

find_library(BASS_LIBRARY NAMES bass HINTS ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64)
find_library(BASSFLAC_LIBRARY NAMES bassflac HINTS ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64)
find_library(BASSOPUS_LIBRARY NAMES bassopus HINTS ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64)

# 3. 실행 파일 빌드 타겟 지정
add_executable(music_player
        main.cpp
        separated_class/MusicPlayerWindow.h
        separated_class/MusicPlayerWindow.cpp
        separated_class/AudioEngine.h
        separated_class/AudioEngine.cpp
        separated_class/MarqueeLabel.h
        separated_class/MarqueeLabel.cpp
)

# 🐧 🌟 [리눅스 치트키] 실행 파일 실행 시 같은 폴더의 .so를 바로 바라보도록 RPATH 설정 강제
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set_target_properties(music_player PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
)

# 4. 헤더 파일 디렉토리 포함
target_include_directories(music_player PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TAGLIB_INCLUDE_DIRS}
)

# 5. 라이브러리 링크
target_link_libraries(music_player PRIVATE
        Threads::Threads
        ${TAGLIB_LIBRARIES}
        ${BASS_LIBRARY}
        ${BASSFLAC_LIBRARY}
        ${BASSOPUS_LIBRARY}
        Qt6::Widgets
)

# 6. 에셋 및 🌟 [경로 수정] linux_x64 안의 .so 파일들을 빌드 폴더로 자동 복사
add_custom_command(TARGET music_player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
        "$<TARGET_FILE_DIR:music_player>/assets/fonts"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
        "$<TARGET_FILE_DIR:music_player>/assets/icons"

        # 리눅스 PC용 폴더에서 빌드 폴더 루트로 복사합니다.
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64/libbass.so"
        "$<TARGET_FILE_DIR:music_player>/libbass.so"

        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64/libbassflac.so"
        "$<TARGET_FILE_DIR:music_player>/libbassflac.so"

        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64/libbassopus.so"
        "$<TARGET_FILE_DIR:music_player>/libbassopus.so"
)