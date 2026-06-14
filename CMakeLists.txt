cmake_minimum_required(VERSION 3.20)
project(music_player CXX)

set(CMAKE_CXX_STANDARD 20)

# Qt 빌드 자동화 시스템 활성화 (MOC, UIC, RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# BASS 오디오 엔진 공통 헤더 파일 경로 포함
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/include)

# ==============================================================================
# 1. 🌐 플랫폼별 빌드 환경 분기 (Android vs Linux PC)
# ==============================================================================
if(ANDROID)
    message(STATUS "[PSMP 클라우드 빌드] 시스템 감지: Android 모바일 타겟")

    # 📱 안드로이드 모바일 UI(QML/Quick) 가동을 위한 핵심 Qt6 패키지 로드
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

    # 안드로이드 스마트폰(ARM 64비트 프로세서) 전용 BASS 라이브러리 경로 지정
    # 💡 팁: BASS for Android 패키지에서 추출한 arm64-v8a 용 .so 파일들이 이 경로에 위치해야 합니다.
    set(BASS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/android/arm64-v8a)

    find_library(BASS_LIBRARY NAMES bass HINTS ${BASS_LIB_DIR})
    find_library(BASSFLAC_LIBRARY NAMES bassflac HINTS ${BASS_LIB_DIR})
    find_library(BASSOPUS_LIBRARY NAMES bassopus HINTS ${BASS_LIB_DIR})

    # ※ 안드로이드 환경에서는 TagLib 라이브러리를 소스 통째로 내장하거나
    # Android용으로 따로 크로스 컴파일해서 링크해야 하므로 우선 분기를 분리해 둡니다.

else()
    message(STATUS "[PSMP 클라우드 빌드] 시스템 감지: Linux PC 데스크톱 타겟")

    # 🐧 리눅스 PC 및 기존 QtWidgets UI 구동을 위한 패키지 로드
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(TAGLIB REQUIRED taglib)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)

    # 리눅스 PC(x86_64) 전용 BASS 라이브러리 경로 지정
    set(BASS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/bass/libs/linux_x64)

    find_library(BASS_LIBRARY NAMES bass HINTS ${BASS_LIB_DIR})
    find_library(BASSFLAC_LIBRARY NAMES bassflac HINTS ${BASS_LIB_DIR})
    find_library(BASSOPUS_LIBRARY NAMES bassopus HINTS ${BASS_LIB_DIR})
endif()


# ==============================================================================
# 2. 📂 소스 코드 파일 빌드 타겟 지정
# ==============================================================================
if(ANDROID)
    # [모바일 버전] QML 화면 진입을 위한 전용 엔트리 포인트(main_android.cpp) 사용 예정
    add_executable(music_player
            main_android.cpp
            separated_class/AudioEngine.h
            separated_class/AudioEngine.cpp
    )
else()
    # [PC 버전] 우리가 기존에 개발해 둔 짱짱한 QtWidgets 소스 파일들 전체 빌드
    add_executable(music_player
            main.cpp
            separated_class/MusicPlayerWindow.h
            separated_class/MusicPlayerWindow.cpp
            separated_class/AudioEngine.h
            separated_class/AudioEngine.cpp
            separated_class/MarqueeLabel.h
            separated_class/MarqueeLabel.cpp
    )
endif()


# ==============================================================================
# 3. 🔗 라이브러리 링크 및 의존성 주입
# ==============================================================================
target_include_directories(music_player PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(ANDROID)
    # 📱 안드로이드 타겟 링크 설정
    target_link_libraries(music_player PRIVATE
            ${BASS_LIBRARY}
            ${BASSFLAC_LIBRARY}
            ${BASSOPUS_LIBRARY}
            Qt6::Core
            Qt6::Gui
            Qt6::Qml
            Qt6::Quick
    )

    # 🔥 [안드로이드 빌드 핵심 치트키]
    # 외부 써드파티 .so 파일들(BASS 플러그인들)을 최종 안드로이드 APK 파일 내부에
    # 자동으로 패키징 및 내장하라고 Qt 배포 툴(androiddeployqt)에 특수 명령을 내립니다!
    set_property(TARGET music_player PROPERTY QT_ANDROID_EXTRA_LIBS
            ${BASS_LIBRARY}
            ${BASSFLAC_LIBRARY}
            ${BASSOPUS_LIBRARY}
    )

else()
    # 🐧 리눅스 PC 타겟 링크 및 로컬 실행 파일 경로 강제 설정 ($ORIGIN RPATH)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set_target_properties(music_player PROPERTIES
            BUILD_WITH_INSTALL_RPATH TRUE
            INSTALL_RPATH "$ORIGIN"
    )

    target_include_directories(music_player PRIVATE ${TAGLIB_INCLUDE_DIRS})
    target_link_libraries(music_player PRIVATE
            Threads::Threads
            ${TAGLIB_LIBRARIES}
            ${BASS_LIBRARY}
            ${BASSFLAC_LIBRARY}
            ${BASSOPUS_LIBRARY}
            Qt6::Widgets
    )

    # 빌드 성공 후 실행 파일 폴더로 폰트, 아이콘, .so 엔진 복사 매크로
    add_custom_command(TARGET music_player POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts"
            "$<TARGET_FILE_DIR:music_player>/assets/fonts"

            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
            "$<TARGET_FILE_DIR:music_player>/assets/icons"

            COMMAND ${CMAKE_COMMAND} -E copy "${BASS_LIBRARY}" "$<TARGET_FILE_DIR:music_player>/"
            COMMAND ${CMAKE_COMMAND} -E copy "${BASSFLAC_LIBRARY}" "$<TARGET_FILE_DIR:music_player>/"
            COMMAND ${CMAKE_COMMAND} -E copy "${BASSOPUS_LIBRARY}" "$<TARGET_FILE_DIR:music_player>/"
    )
endif()