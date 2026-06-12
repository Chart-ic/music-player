#pragma once

#include <vector>
#include <GLFW/glfw3.h> // OpenGL 설정을 위해 필요

// 이미지 디코딩을 위한 stb_image 라이브러리
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>  // 💡 큰따옴표 지우고 시스템 헤더인 <stb_image.h>로 변경!

class TextureLoader {
public:
    // 💡 바이너리 데이터(std::vector)를 받아서 OpenGL 텍스처로 변환하는 함수
    static GLuint loadTextureFromMemory(const std::vector<uint8_t>& data, int& outWidth, int& outHeight) {
        if (data.empty()) return 0;

        // 1. stb_image를 이용해 메모리에 있는 JPEG/PNG 바이너리를 픽셀 데이터로 디코딩
        int channels;
        unsigned char* pixels = stbi_load_from_memory(
            data.data(),
            static_cast<int>(data.size()),
            &outWidth,
            &outHeight,
            &channels,
            4 // RGBA 4채널로 강제 고정
        );

        if (pixels == nullptr) {
            return 0; // 디코딩 실패 시 0 반환
        }

        // 2. OpenGL 텍스처 생성 및 바인딩
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 3. 텍스처 필터링 및 래핑 설정 (이미지가 축소/확대될 때 부드럽게)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 4. 그래픽 카드 메모리에 픽셀 데이터 업로드
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            outWidth,
            outHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels
        );

        // 5. 사용한 CPU 메모리 해제 및 언바인딩
        stbi_image_free(pixels);
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

    // 💡 프로그램 종료 시 텍스처를 그래픽 카드에서 안전하게 지워주는 함수
    static void freeTexture(GLuint& textureID) {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }
};