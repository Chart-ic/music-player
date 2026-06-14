#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <string>
#include "../libs/bass/bass.h" // 🌟 대망의 BASS 헤더 장착!
#include <QBitArray>
// 기존에 사용하던 메타데이터 구조체 유지
struct MusicMetadata {
    std::string title;
    std::string artist;
    std::string album;
    int bitrate = 0;
    int sampleRate = 0;
    int channels = 0;
    int bitDepth = 0;
    int track = 0;         // 🌟 추가: 트랙 번호
    int disc = 0;          // 🌟 추가: 디스크 번호
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


    static QByteArray getAlbumArt(const std::string& filePath); // 🌟 앨범 아트 추출 함수 추가!

    // volume
    void setVolume(float volume); // 0.0 ~ 1.0 사이의 볼륨 값

private:
    HSTREAM currentStream; // 🌟 BASS에서 재생 중인 곡을 제어하는 핸들(ID) 변수
};

#endif // AUDIOENGINE_H