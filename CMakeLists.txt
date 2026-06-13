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

# 2. BASS 오디오 라이브러리 경로 및 위치 찾기
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/lib)
find_library(BASS_LIBRARY
        NAMES bass
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/lib
        NO_DEFAULT_PATH
)

# 3. 실행 파일 빌드 타겟 지정 (.h와 .cpp 누락 없이 명시)
add_executable(music_player
        main.cpp
        separated_class/MusicPlayerWindow.h
        separated_class/MusicPlayerWindow.cpp
        separated_class/AudioEngine.h
        separated_class/AudioEngine.cpp
)

# 4. 헤더 파일 디렉토리 포함
target_include_directories(music_player PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TAGLIB_INCLUDE_DIRS}
)

# 5. 라이브러리 링크 (난잡했던 구형 사운드 API 링킹 삭제, BASS 및 TagLib 수렴)
target_link_libraries(music_player PRIVATE
        Threads::Threads
        ${TAGLIB_LIBRARIES}
        ${BASS_LIBRARY}
        Qt6::Widgets
)

# 빌드할 때와 나중에 인스톨/실행할 때 동일한 RPATH 규칙을 강제 적용합니다.
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set_target_properties(music_player PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"  # 🌟 실행 파일 바로 옆방에서 libbass.so를 무조건 찾으라는 리눅스 절대 규칙
)

# 7. 포트, 아이콘 에셋 복사 + 🌟 libbass.so 빌드 폴더 자동 복사 추가!
add_custom_command(TARGET music_player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
        "$<TARGET_FILE_DIR:music_player>/assets/fonts"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
        "$<TARGET_FILE_DIR:music_player>/assets/icons"

        # 컴파일이 끝날 때마다 빌드 결과물 폴더로 libbass.so를 알아서 던져주는 똑똑한 자동화 코드!
        COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/lib/libbass.so"
        "$<TARGET_FILE_DIR:music_player>/libbass.so"
)