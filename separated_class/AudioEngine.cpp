#define MINIAUDIO_IMPLEMENTATION
#include "AudioEngine.h"
#include <iostream>
#include <algorithm>

// taglib
#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/audioproperties.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacproperties.h>
#include <taglib/wavproperties.h>

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    if (m_isLoaded) {
        ma_device_uninit(&device);
        ma_decoder_uninit(&m_decoder);
    }
}

// 3. Load 함수 (음원 바뀔 때마다 장치 샘플레이트 맞춤 변신!)
bool AudioEngine::load(const std::string& filePath) {
    // 이전 곡이 켜져있다면 장치 끄기
    if (m_isLoaded) {
        ma_device_uninit(&device);
        ma_decoder_uninit(&m_decoder);
        m_isLoaded = false;
    }

    // [STEP 1] 디코더 열기 (32-bit Float 강제, 샘플레이트는 원본 따름(0))
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 0);
    if (ma_decoder_init_file(filePath.c_str(), &decoderConfig, &m_decoder) != MA_SUCCESS) {
        std::cerr << "디코더 초기화 실패!" << std::endl;
        return false;
    }

    // [STEP 2] 스피커 장치 열기 (디코더에서 알아낸 '진짜 샘플레이트'로 엽니다!)
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = m_decoder.outputChannels;
    deviceConfig.sampleRate = m_decoder.outputSampleRate; // 여기가 44100, 96000 등으로 휙휙 바뀜!
    deviceConfig.dataCallback = AudioEngine::data_callback;
    deviceConfig.pUserData = this;

    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        ma_decoder_uninit(&m_decoder);
        std::cerr << "장치 초기화 실패!" << std::endl;
        return false;
    }

    m_isLoaded = true;
    return true;
}

// 4. 기본 조작 함수들 (ma_engine에서 ma_device 조작으로 변경)
void AudioEngine::play() { if (m_isLoaded) ma_device_start(&device); }
void AudioEngine::pause() { if (m_isLoaded) ma_device_stop(&device); }
void AudioEngine::resume() { play(); }
void AudioEngine::stop() { pause(); setPosition(0); }
void AudioEngine::setVolume(float volume) { ma_device_set_master_volume(&device, volume); }

bool AudioEngine::isPlaying() const {
    return m_isLoaded && ma_device_get_state(&const_cast<ma_device&>(device)) == ma_device_state_started;
}

// 5. 시간 & 탐색 기능 (프레임 기반으로 정확하게)
void AudioEngine::setPosition(float seconds) { // (함수 이름은 작성하신 코드에 맞게!)
    if (!m_isLoaded) return;
    // 1. 오디오 장치 아주 잠깐 일시정지 (스레드 충돌 방지)
    ma_device_stop(&device);

    // 2. 원하는 위치로 이동 (기존에 있던 코드)
    auto targetFrame = static_cast<ma_uint64>(seconds * static_cast<float>(m_decoder.outputSampleRate));
    ma_data_source_seek_to_pcm_frame(&m_decoder, targetFrame);

    // 3. 다시 재생 시작
    ma_device_start(&device);
}

float AudioEngine::getTotalTime() const {
    if (!m_isLoaded) return 0.0f;
    ma_uint64 length;
    ma_decoder_get_length_in_pcm_frames(&const_cast<ma_decoder&>(m_decoder), &length);
    return static_cast<float>(length) / static_cast<float>(m_decoder.outputSampleRate);
}

float AudioEngine::getCurrentTime() const {
    if (!m_isLoaded) return 0.0f;
    ma_uint64 cursor;
    ma_decoder_get_cursor_in_pcm_frames(&const_cast<ma_decoder&>(m_decoder), &cursor);
    return static_cast<float>(cursor) / static_cast<float>(m_decoder.outputSampleRate);
}


// 118번 줄 부근 (콜백 구현)
void AudioEngine::data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount) { // NOLINT
    auto *engine = static_cast<AudioEngine*>(pDevice->pUserData);
    if (!engine || !engine->m_isLoaded) return;

    // 디코더에서 프레임 읽어오기
    ma_decoder_read_pcm_frames(&engine->m_decoder, pOutput, frameCount, nullptr);
}

MusicMetadata AudioEngine::getMetadata(const std::string& filePath) {
    MusicMetadata meta;
    meta.title = "Unknown Title";
    meta.artist = "Unknown Artist";
    meta.album = "Unknown Album";
    meta.duration = 0;
    meta.track = 0;
    meta.disc = 0;
    meta.channels = 2;
    meta.bitDepth = 16; // 기본값
    meta.sampleRate = 44100;
    meta.bitrate = 0;

    TagLib::FileRef f(filePath.c_str());
    if (!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        if (!tag->title().isEmpty()) meta.title = tag->title().toCString(true);
        if (!tag->artist().isEmpty()) meta.artist = tag->artist().toCString(true);
        if (!tag->album().isEmpty()) meta.album = tag->album().toCString(true);
        meta.track = static_cast<int>(tag->track());
    }

    if (!f.isNull() && f.audioProperties()) {
        TagLib::AudioProperties *properties = f.audioProperties();
        meta.duration = properties->lengthInSeconds();
        meta.bitrate = properties->bitrate();
        meta.sampleRate = properties->sampleRate();
        meta.channels = properties->channels();

        // FLAC 파일인 경우 24bit / 32bit 등 진짜 비트 심도 추출
        if (auto *flacProps = dynamic_cast<TagLib::FLAC::Properties *>(properties)) {
            meta.bitDepth = flacProps->bitsPerSample();
        }
        // WAV 파일인 경우
        else if (auto *wavProps = dynamic_cast<TagLib::RIFF::WAV::Properties *>(properties)) {
            meta.bitDepth = wavProps->bitsPerSample();
        }
    }
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_unknown, 0, 0);
    ma_decoder decoder;

    if (ma_decoder_init_file(filePath.c_str(), &decoderConfig, &decoder) == MA_SUCCESS) {

        // 2. 원본 음원의 Bit Depth 판별
        int bitDepth = 16;
        switch (decoder.outputFormat) {
            case ma_format_s16:
                bitDepth = 16;
                break;
            case ma_format_s24: // 보통 24bit 음원은 이 포맷으로 들어옴!
                bitDepth = 24;
                break;
            case ma_format_s32:
            case ma_format_f32:
                bitDepth = 32;
                break;
            default:
                bitDepth = 16;
                break;
        }

        // meta 구조체에 값 대입
        meta.bitDepth = bitDepth;
        meta.sampleRate = static_cast<int>(decoder.outputSampleRate);
        meta.channels = static_cast<int>(decoder.outputChannels);

        // 디코더 사용 후 해제
        ma_decoder_uninit(&decoder);
    }

    return meta;
}

QByteArray AudioEngine::getAlbumArt(const std::string& filePath) {
    // 파일 확장자 추출 및 소문자 변환
    std::string ext;
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        ext = filePath.substr(dotPos + 1);
        std::ranges::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    // 1. MP3 파일인 경우에만 MPEG 파서 실행 (로그 출력 방지)
    if (ext == "mp3") {
        TagLib::MPEG::File mpegFile(filePath.c_str());
        if (mpegFile.isValid() && mpegFile.ID3v2Tag()) {
            TagLib::ID3v2::Tag *id3v2tag = mpegFile.ID3v2Tag();
            TagLib::ID3v2::FrameList frames = id3v2tag->frameListMap()["APIC"];
            if (!frames.isEmpty()) {
                if (auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frames.front())) {
                    return {frame->picture().data(), static_cast<int>(frame->picture().size())};
                }
            }
        }
    }
    // 2. FLAC 파일인 경우
    else if (ext == "flac") {
        TagLib::FLAC::File flacFile(filePath.c_str());
        if (flacFile.isValid()) {
            const TagLib::List<TagLib::FLAC::Picture *> pictureList = flacFile.pictureList();
            if (!pictureList.isEmpty()) {
                TagLib::FLAC::Picture *picture = pictureList.front();
                return {picture->data().data(), static_cast<int>(picture->data().size())};
            }
        }
    }

    return {};
}

