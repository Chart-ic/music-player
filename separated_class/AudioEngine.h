#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <string>
#include "../libs/bass/bass.h" // 🌟 대망의 BASS 헤더 장착!

// 기존에 사용하던 메타데이터 구조체 유지
struct MusicMetadata {
    std::string title;
    std::string artist;
    std::string album;
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool load(const std::string& filePath);
    void play();
    void pause();
    void setPosition(double seconds);

    double getCurrentTime() const;
    double getTotalTime() const;

    // 플레이리스트 불러올 때 썼던 정적 함수 유지
    static MusicMetadata getMetadata(const std::string& filePath);

private:
    HSTREAM currentStream; // 🌟 BASS에서 재생 중인 곡을 제어하는 핸들(ID) 변수
};

#endif // AUDIOENGINE_H