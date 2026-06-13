cmake_minimum_required(VERSION 3.20)
project(music_player CXX)

set(CMAKE_CXX_STANDARD 20)

# Qt 메타 오브젝트 컴파일러 자동화 켜기
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 1. 필수 라이브러리 찾기 (GLFW, OpenGL 삭제!)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 2. 실행 파일 생성 (ImGui 소스들 싹 다 삭제!)
add_executable(music_player
        main.cpp
        separated_class/AudioEngine.h
)

# 3. 헤더 파일 경로 지정
target_include_directories(music_player PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TAGLIB_INCLUDE_DIRS}
)

# 4. 링크할 라이브러리 지정 (Qt6 및 태그립, 오디오만 남김)
target_link_libraries(music_player PRIVATE
        Threads::Threads
        asound
        pulse
        m
        pthread
        dl
        ${TAGLIB_LIBRARIES}
        Qt6::Widgets
)

# 5. 빌드 시 폰트 및 아이콘 에셋 복사 (이건 잘 만들어둬서 그대로 유지!)
add_custom_command(TARGET music_player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
        "$<TARGET_FILE_DIR:music_player>/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
        "$<TARGET_FILE_DIR:music_player>/assets/icons"
)