#pragma once

#include <string>
#include <mutex> // 스레드 동기화를 위해 뮤텍스 헤더 추가!
#include "third_party/miniaudio.h"

// TagLib 헤더
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>

struct MusicMetadata {
    std::string title = "Unknown Title";
    std::string artist = "Unknown Artist";
    std::string album = "Unknown Album";
    std::vector<uint8_t> albumArtData; // 앨범 아트 원본 데이터 (JPEG/PNG)
    bool hasAlbumArt = false;
};

class AudioEngine {
private:
    ma_decoder m_decoder{};
    ma_device m_device{};
    bool m_isInitialized = false;
    std::mutex m_decoderMutex; // 디코더 접근을 보호할 마법의 자물쇠!

    static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) { // NOLINT
        auto* engine = static_cast<AudioEngine*>(pDevice->pUserData);
        if (engine == nullptr || !engine->m_isInitialized) return;

        // 오디오 스레드가 데이터를 읽는 동안 UI 스레드가 접근하지 못하게 잠금!
        std::lock_guard<std::mutex> lock(engine->m_decoderMutex);

        ma_uint64 framesRead;
        ma_decoder_read_pcm_frames(&engine->m_decoder, pOutput, frameCount, &framesRead);
        (void)pInput;
    }

public:
    AudioEngine() = default;
    ~AudioEngine() { uninit(); }

    // 💡 FLAC 파일에서 메타데이터와 앨범 아트를 쏙 뽑아오는 마법의 함수 추가!
    static MusicMetadata getMetadata(const std::string& filePath) {
        MusicMetadata meta;

        // 1. TagLib로 파일 열기 (UTF-8 경로 처리를 위해 리눅스/C++ 표준 방식 사용)
        TagLib::FileRef f(filePath.c_str());
        if (!f.isNull() && f.tag()) {
            TagLib::Tag* tag = f.tag();

            // 정보가 비어있지 않다면 변환해서 넣어줌 (.to8Bit(true)는 UTF-8 보존을 위함!)
            if (!tag->title().isEmpty())  meta.title  = tag->title().to8Bit(true);
            if (!tag->artist().isEmpty()) meta.artist = tag->artist().to8Bit(true);
            if (!tag->album().isEmpty())  meta.album  = tag->album().to8Bit(true);
        }

        // 2. 대망의 FLAC 전용 앨범 아트(Picture) 추출!
        TagLib::FLAC::File flacFile(filePath.c_str());
        if (flacFile.isValid()) {
            const TagLib::List<TagLib::FLAC::Picture*>& pictureList = flacFile.pictureList();
            if (!pictureList.isEmpty()) {
                // 맨 첫 번째 사진(보통 전면 표지)을 가져옴
                TagLib::FLAC::Picture* pic = pictureList.front();

                // 바이너리 데이터 벡터에 고스란히 복사
                const TagLib::ByteVector& data = pic->data();
                meta.albumArtData.assign(data.data(), data.data() + data.size());
                meta.hasAlbumArt = true;
            }
        }

        return meta;
    }

    float getCurrentTime() {
        if (!m_isInitialized) return 0.0f;
        // 안전하게 자물쇠를 잠그고 현재 위치 읽기
        std::lock_guard<std::mutex> lock(m_decoderMutex);
        ma_uint64 cursor;
        ma_decoder_get_cursor_in_pcm_frames(&m_decoder, &cursor);
        return static_cast<float>(cursor) / static_cast<float>(m_decoder.outputSampleRate);
    }

    float getTotalTime() {
        if (!m_isInitialized) return 0.0f;
        // 안전하게 자물쇠를 잠그고 총 길이 읽기
        std::lock_guard<std::mutex> lock(m_decoderMutex);
        ma_uint64 length;
        ma_decoder_get_length_in_pcm_frames(&m_decoder, &length);
        return static_cast<float>(length) / static_cast<float>(m_decoder.outputSampleRate);
    }

    void seekTo(float timeInSeconds) {
        if (!m_isInitialized) return;
        // UI 스레드가 탐색(Seek)을 하는 동안 오디오 스레드가 읽지 못하게 잠금!
        std::lock_guard<std::mutex> lock(m_decoderMutex);
        auto targetFrame = static_cast<ma_uint64>(timeInSeconds * static_cast<float>(m_decoder.outputSampleRate));
        ma_decoder_seek_to_pcm_frame(&m_decoder, targetFrame);
    }

    bool load(const std::string& filePath) {
        uninit();

        std::lock_guard<std::mutex> lock(m_decoderMutex);
        ma_result result = ma_decoder_init_file(filePath.c_str(), nullptr, &m_decoder);
        if (result != MA_SUCCESS) return false;

        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format   = m_decoder.outputFormat;
        deviceConfig.playback.channels = m_decoder.outputChannels;
        deviceConfig.sampleRate        = m_decoder.outputSampleRate;
        deviceConfig.dataCallback      = data_callback;
        deviceConfig.pUserData         = this;

        result = ma_device_init(nullptr, &deviceConfig, &m_device);
        if (result != MA_SUCCESS) {
            ma_decoder_uninit(&m_decoder);
            return false;
        }

        m_isInitialized = true;
        return true;
    }

    void play() { if (m_isInitialized) ma_device_start(&m_device); }
    void pause() { if (m_isInitialized) ma_device_stop(&m_device); }
    void setVolume(float volume) { if (m_isInitialized) ma_device_set_master_volume(&m_device, volume); }

    void uninit() {
        if (m_isInitialized) {
            m_isInitialized = false;
            ma_device_uninit(&m_device); // 1. 오디오 장치 스레드를 먼저 안전하게 종료 (대기)

            std::lock_guard<std::mutex> lock(m_decoderMutex); // 2. 그 후 안전하게 디코더 해제
            ma_decoder_uninit(&m_decoder);
        }
    }
};