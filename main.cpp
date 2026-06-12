// NOLINTNEXTLINE(llvm-header-guard)
#include <string>

// 여기서 단 한 번만 Miniaudio 내부 구현체를 생성!
#define MINIAUDIO_IMPLEMENTATION
#include "separated_class/AudioEngine.h"

// imgui 라이브러리
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// other
#include "separated_class/TextureLoader.h"

void ApplyTrendyDarkStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // ✨ 1. 모서리 둥글기 튜닝 (Rounding)
        style.WindowRounding = 12.0f;     // 메인 윈도우 모서리 둥글게
        style.FrameRounding = 8.0f;       // 버튼, 슬라이더 모서리 둥글게
        style.PopupRounding = 8.0f;       // 팝업 창 모서리
        style.GrabRounding = 6.0f;        // 슬라이더 조절 바 모서리

        // ✨ 2. 여백 및 간격 조절 (Padding)
        style.WindowPadding = ImVec2(20.0f, 20.0f); // 창 내부 여백을 넉넉하게
        style.FramePadding = ImVec2(12.0f, 8.0f);   // 버튼 내부 텍스트 여백 늘리기
        style.ItemSpacing = ImVec2(15.0f, 12.0f);   // 위아래 컴포넌트 간격 넓히기

        // ✨ 3. 세련된 딥 다크 컬러 피킹 (Colors)
        colors[ImGuiCol_WindowBg]             = ImVec4(0.09f, 0.10f, 0.15f, 1.00f); // 깊은 네이비 블랙 배경
        colors[ImGuiCol_ChildBg]              = ImVec4(0.14f, 0.15f, 0.20f, 1.00f); // 내부 자식 창 배경
        colors[ImGuiCol_PopupBg]              = ImVec4(0.14f, 0.15f, 0.20f, 1.00f);

        // 버튼 색상 (평소 / 마우스 올렸을 때 / 클릭했을 때)
        colors[ImGuiCol_Button]               = ImVec4(0.20f, 0.24f, 0.35f, 1.00f); // 차분한 네이비 블루
        colors[ImGuiCol_ButtonHovered]        = ImVec4(0.28f, 0.33f, 0.48f, 1.00f); // 밝은 블루
        colors[ImGuiCol_ButtonActive]         = ImVec4(0.38f, 0.43f, 0.60f, 1.00f);

        // 헤더 및 타이틀 바 색상
        colors[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.10f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgActive]        = ImVec4(0.14f, 0.15f, 0.20f, 1.00f);
        colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.24f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.33f, 0.48f, 1.00f);
        colors[ImGuiCol_HeaderActive]         = ImVec4(0.38f, 0.43f, 0.60f, 1.00f);

        // 텍스트 색상
        colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f); // 완전 흰색보단 눈이 편한 아이보리 화이트
        colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.55f, 0.65f, 1.00f);
    }

int main() {
    // 1. GLFW 초기화
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "Hi-Res Music Player", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 2. ImGui 컨텍스트 초기화
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // 💡 [여기서부터 추가!] 한글 폰트 로드하기
    // 아치 리눅스에 설치된 나눔고딕 폰트 경로 지정 (크기는 18 픽셀이 보기 좋아!)
    const char* fontPath = "fonts/PretendardJP-Thin.ttf";

    // ImFontConfig 설정을 통해 한글 글꼴 범위를 지정해 줘야 해.
    ImFontConfig config;
    config.MergeMode = false; // 기존 폰트에 병합할 게 아니라 새로 덮어씌움

    // 🌟 io.Fonts->GetGlyphRangesKorean() 이게 바로 한국어(? 표현방지) 치트키!
    ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, &config, io.Fonts->GetGlyphRangesKorean());

    if (font == nullptr) {
        // 혹시 시스템 경로에 폰트가 없으면 에러 메시지 띄우기
        printf("❌ 한글 폰트를 로드하지 못했습니다! 경로를 확인하세요.\n");
    }

    ImGui::StyleColorsDark();

    // icon load
    int imgW = 0, imgH = 0;

    // 💡 FromMemory -> FromFile 로 이름이 바뀐 걸 꼭 확인해!
    GLuint iconSong   = TextureLoader::loadTextureFromFile("assets/icons/song.png", imgW, imgH);
    GLuint iconArtist = TextureLoader::loadTextureFromFile("assets/icons/artist.png", imgW, imgH);
    GLuint iconAlbum  = TextureLoader::loadTextureFromFile("assets/icons/album.png", imgW, imgH);
    GLuint iconPlay   = TextureLoader::loadTextureFromFile("assets/icons/play.png", imgW, imgH);
    GLuint iconPause  = TextureLoader::loadTextureFromFile("assets/icons/pause.png", imgW, imgH);


    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 3. 분리한 오디오 엔진 생성 및 노래 로드
    AudioEngine player;
    std::string songPath = "test.flac";
    player.load(songPath);

    MusicMetadata currentMeta = AudioEngine::getMetadata(songPath);

    // 💡 앨범 아트를 위한 OpenGL 텍스처 변수 선언
    GLuint albumArtTexture = 0;
    if (currentMeta.hasAlbumArt) {
        int artWidth = 0;
        int artHeight = 0;
        albumArtTexture = TextureLoader::loadTextureFromMemory(currentMeta.albumArtData, artWidth, artHeight);
    }

    float volume = 1.0f;
    static float sliderTime = 0.0f; // 매 프레임 시간이 초기화되지 않도록 보관하는 정적 변수

    ApplyTrendyDarkStyle();

    // 4. 메인 렌더링 루프
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 🌟 [치트키 1] 창의 타이틀바, 크기 조절, 이동, 접기 기능을 다 꺼버리는 플래그 세팅!
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoCollapse;

        // 🌟 [치트키 2] ImGui 창의 위치를 무조건 (0, 0) 즉, 왼쪽 맨 위 구석으로 고정!
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));

        // 🌟 [치트키 3] ImGui 창의 크기를 GLFW 바깥 창 크기(io.DisplaySize)와 100% 똑같이 강제 일치!
        ImGui::SetNextWindowSize(io.DisplaySize);

        // 💡 이제 위에서 세팅한 플래그(window_flags)를 매개변수로 넘겨주며 창을 열어!
        ImGui::Begin("Music Player", nullptr, window_flags);

        // 1. 좌측: 대형 앨범 아트 배치 (기존 코드)
        if (albumArtTexture != 0) {
            auto textureID = static_cast<ImTextureID>(static_cast<uintptr_t>(albumArtTexture));
            ImGui::Image(textureID, ImVec2(180, 180));
        } else {
            ImGui::Dummy(ImVec2(180, 180));
        }

        ImGui::SameLine();

        // 2. 우측: 곡 정보 및 컨트롤 버튼
        ImGui::BeginGroup();

            // 🎵 곡 제목 (앞에 song.png 아이콘 배치)
            if (iconSong != 0) {
                ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconSong)), ImVec2(20, 20));
                ImGui::SameLine();
            }
            ImGui::Text("%s", currentMeta.title.c_str());

            // 👤 아티스트 (앞에 artist.png 아이콘 배치)
            if (iconArtist != 0) {
                ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconArtist)), ImVec2(20, 20));
                ImGui::SameLine();
            }
            ImGui::TextDisabled("%s", currentMeta.artist.c_str());

            // 💿 앨범명 (앞에 album.png 아이콘 배치)
            if (iconAlbum != 0) {
                ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(iconAlbum)), ImVec2(20, 20));
                ImGui::SameLine();
            }
            ImGui::TextDisabled("%s", currentMeta.album.c_str());

            ImGui::Spacing(); ImGui::Spacing(); // 숨통 틔우기 여백

        // Spacing(); Spacing(); 밑에 있는 버튼 영역을 이걸로 교체!

        // ▶️ PLAY 이미지 버튼
        if (iconPlay != 0) {
            auto playID = static_cast<ImTextureID>(static_cast<uintptr_t>(iconPlay));

            // 💡 [찌그러짐 방지 치트키] 이 버튼만 임시로 내부 여백을 사방 4픽셀로 좁히기!
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

            if (ImGui::ImageButton("##PlayBtn", playID, ImVec2(40, 40))) {
                // 💡 [실제 작동 코드 삽입!] 오디오 엔진 재생 시작
                // (※ 네 AudioEngine 클래스 내부 재생 함수 이름이 play() 또는 start() 인지 확인하고 맞춰줘!)
                player.play();
            }

            ImGui::PopStyleVar(); // 임시 스타일 해제 (원상복구)
        }

        ImGui::SameLine();

        // ⏸️ PAUSE 이미지 버튼
        if (iconPause != 0) {
            auto pauseID = static_cast<ImTextureID>(static_cast<uintptr_t>(iconPause));

            // 💡 여기도 똑같이 여백 간섭 제거
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

            if (ImGui::ImageButton("##PauseBtn", pauseID, ImVec2(40, 40))) {
                // 💡 [실제 작동 코드 삽입!] 오디오 엔진 일시정지
                // (※ 내부 함수 이름이 pause() 또는 stop() 인지 확인!)
                player.pause();
            }

            ImGui::PopStyleVar();
        }

        ImGui::EndGroup();

        if (ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f)) {
            player.setVolume(volume);
        }

        ImGui::Separator();

        // --- 💡 재생 바(Seek Bar) 영역 시작 ---
        float totalTime = player.getTotalTime();

        // 1. [시간 계산 및 텍스트 출력]
        int curMin = static_cast<int>(sliderTime) / 60;
        int curSec = static_cast<int>(sliderTime) % 60;
        int totMin = static_cast<int>(totalTime) / 60;
        int totSec = static_cast<int>(totalTime) % 60;
        ImGui::Text("%02d:%02d / %02d:%02d", curMin, curSec, totMin, totSec);

        // 2. [슬라이더 그리기]
        ImGui::SliderFloat("Position", &sliderTime, 0.0f, totalTime, "");

        // 3. [마우스를 놓는 순간 실제 오디오 탐색(Seek) 수행]
        // 💡 오디오 위치 이동을 "시간 동기화"보다 먼저 처리하는 게 핵심!
        bool justDeactivated = ImGui::IsItemDeactivatedAfterEdit();
        if (justDeactivated) {
            player.seekTo(sliderTime);
        }

        // 4. [슬라이더 마우스 조작 상태 체크 및 동기화]
        // 💡 사용자가 드래그 중이거나, '방금 막 마우스를 놓은 순간(justDeactivated)'이 아닐 때만 실제 시간을 동기화해!
        if (!ImGui::IsItemActive() && !justDeactivated) {
            sliderTime = player.getCurrentTime();
        }
        // --- 재생 바 영역 끝 ---

        ImGui::End();

        // 렌더링
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 5. 종료 및 자원 해제
    player.uninit();
    TextureLoader::freeTexture(albumArtTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}