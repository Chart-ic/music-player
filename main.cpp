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

    void ApplyTrendyDarkStyle(); {
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

    // 4. 메인 렌더링 루프
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Music Controller");

        // 💡 [새로 추가] 상단 메타데이터 출력부
        ImGui::Spacing();
        ImGui::Text("🎵 곡 제목 : %s", currentMeta.title.c_str());
        ImGui::Text("👤 아티스트: %s", currentMeta.artist.c_str());
        ImGui::Text("💿 앨범명   : %s", currentMeta.album.c_str());

        // 💡 앨범 아트가 있으면 화면에 이쁘게 이미지 띄우기!
        if (albumArtTexture != 0) {
            // 가로세로 200x200 크기로 이쁘게 제한해서 그리기
            // 💡 auto와 C++ 캐스팅의 완벽한 조합! 어떤 ImTextureID 정의형태든 다 뚫어버림.
            auto textureID = static_cast<ImTextureID>(static_cast<uintptr_t>(albumArtTexture));
            ImGui::Image(textureID, ImVec2(200, 200));
        } else {
            // 앨범 아트가 없을 때 보일 더미 사각형 상자
            ImGui::Dummy(ImVec2(200, 200));
            ImGui::SameLine();
            ImGui::Text("❌ 앨범 아트 없음");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Hi-Res Audio Player에 오신 것을 환영합니다!");


        if (ImGui::Button("PLAY (재생)")) { player.play(); }
        ImGui::SameLine();
        if (ImGui::Button("PAUSE (일시정지)")) { player.pause(); }

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