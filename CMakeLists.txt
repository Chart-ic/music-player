cmake_minimum_required(VERSION 3.20)
project(music_player CXX)

set(CMAKE_CXX_STANDARD 20)

# 1. GLFW, OpenGL, TagLib 라이브러리 찾기
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)

# 2. ImGui 핵심 소스 파일들 등록
set(IMGUI_DIR "third_party/imgui")
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        # GLFW랑 OpenGL3를 사용하기 위한 백엔드 소스
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 3. 실행 파일 생성 (타겟 이름: music_player)
add_executable(music_player
        main.cpp
        separated_class/AudioEngine.h
        separated_class/TextureLoader.h
        ${IMGUI_SOURCES}
)

# 4. 헤더 파일 경로(Include) 지정 ➡️ 💡 타겟 이름을 'music_player'로 수정!
target_include_directories(music_player PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${TAGLIB_INCLUDE_DIRS}
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        /usr/include/stb
)

# 5. 링크할 라이브러리 지정 ➡️ 💡 타겟 이름을 'music_player'로 수정!
target_link_libraries(music_player PRIVATE
        glfw
        OpenGL::GL
        Threads::Threads
        asound
        pulse
        m
        pthread
        dl
        ${TAGLIB_LIBRARIES} # 💡 PkgConfig로 긁어온 TagLib 링크 바이너리
)

# 💡 빌드할 때 fonts 폴더를 실행 파일 위치로 자동 복사하는 명령어
add_custom_command(TARGET music_player POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
        "$<TARGET_FILE_DIR:music_player>/fonts"
        # 아이콘 복사
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
        "$<TARGET_FILE_DIR:music_player>/assets/icons"
)