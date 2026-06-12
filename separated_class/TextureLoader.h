#pragma once

#include <vector>
#include <string>
#include <GLFW/glfw3.h> // OpenGL 설정을 위해 필요

// 이미지 디코딩을 위한 stb_image 라이브러리
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

class TextureLoader {
public:
    // 💡 1. 바이너리 데이터(std::vector)를 받아서 OpenGL 텍스처로 변환하는 함수 (기존 코드)
    static GLuint loadTextureFromMemory(const std::vector<uint8_t>& data, int& outWidth, int& outHeight) {
        if (data.empty()) return 0;

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
            return 0;
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA,
            outWidth, outHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels
        );

        stbi_image_free(pixels);
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

    // 💡 [새로 추가된 함수!] 일반 파일 경로를 주면 읽어서 OpenGL 텍스처로 변환하는 함수
    static GLuint loadTextureFromFile(const std::string& filePath, int& outWidth, int& outHeight) {
        int channels;
        // 🌟 메모리가 아니라 실제 파일 경로를 찔러넣어서 디코딩함!
        unsigned char* pixels = stbi_load(filePath.c_str(), &outWidth, &outHeight, &channels, 4);

        if (pixels == nullptr) {
            // 콘솔에 어떤 파일 로드에 실패했는지 알려주면 디버깅할 때 편해!
            printf("❌ 이미지 파일 로드 실패: %s\n", filePath.c_str());
            return 0;
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // 아이콘 이미지도 자글자글 깨지지 않게 부드러운 필터 적용
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA,
            outWidth, outHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, pixels
        );

        stbi_image_free(pixels);
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureID;
    }

    // 💡 프로그램 종료 시 텍스처를 안전하게 지워주는 함수 (기존 코드)
    static void freeTexture(GLuint& textureID) {
        if (textureID != 0) {
            glDeleteTextures(1, &textureID);
            textureID = 0;
        }
    }
};