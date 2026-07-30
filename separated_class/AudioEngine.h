#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <string>
#include <QByteArray>
#include "../libs/miniaudio.h"

struct MusicMetadata {
    std::string title;
    std::string artist;
    std::string album;
    int duration{0};
    int track{0};
    int disc{0};
    int channels{2};
    int bitDepth{16};
    int sampleRate{48000};
    int bitrate{320};
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    // initDevice()는 이제 load() 안에서 동적으로 처리하므로 삭제해도 됨!

    bool load(const std::string& filePath);
    void play();
    void pause();
    void resume();
    void stop();
    void setVolume(float volume);
    void setPosition(float seconds);

    [[nodiscard]] float getTotalTime() const;
    [[nodiscard]] float getCurrentTime() const;
    [[nodiscard]] bool isPlaying() const;

    static MusicMetadata getMetadata(const std::string& filePath);
    static QByteArray getAlbumArt(const std::string& filePath);

private:
    ma_device device{};
    ma_decoder m_decoder{}; // 핵심! 엔진 대신 디코더를 직접 씁니다.
    bool m_isLoaded{false};

    // 디바이스 콜백 함수
    static void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
};

#endif