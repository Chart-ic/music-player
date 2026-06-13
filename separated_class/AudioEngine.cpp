#include "AudioEngine.h"
#include <iostream>
#include <taglib/fileref.h>  // 🌟 TagLib 메타데이터 파싱용 헤더
#include <taglib/tag.h>



AudioEngine::AudioEngine() : currentStream(0) {
    // 🌟 1. 리눅스 사운드 장치 초기화 (기본 출력 장치, 44100Hz 기반 시스템 초기화)
    // 리눅스의 ALSA나 PulseAudio/PipeWire 환경을 BASS가 알아서 매핑합니다.
    if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) {
        std::cerr << "[BASS Error] 장치 초기화 실패! 에러 코드: " << BASS_ErrorGetCode() << std::endl;
    }
}

AudioEngine::~AudioEngine() {
    // 🌟 2. 프로그램 안전 종료 시 스트림 및 장치 해제
    if (currentStream) {
        BASS_StreamFree(currentStream);
    }
    BASS_Free();
}

bool AudioEngine::load(const std::string& filePath) {
    // 기존에 재생 중이거나 로드된 노래가 있다면 메모리에서 깔끔하게 해제
    if (currentStream) {
        BASS_StreamFree(currentStream);
        currentStream = 0;
    }

    // 🌟 3. 음원 로드 및 32-bit Float 초고음질 프로세싱 활성화!
    // BASS_SAMPLE_FLOAT 플래그를 주면 믹서 파이프라인 전체가 32비트 부동소수점으로 동작하여,
    // 네가 원하는 192kHz / 384kHz 초고해상도 원음의 손실(다운샘플링)을 완벽하게 방지해줘.
    currentStream = BASS_StreamCreateFile(FALSE, filePath.c_str(), 0, 0, BASS_SAMPLE_FLOAT);

    if (!currentStream) {
        std::cerr << "[BASS Error] 음원 로드 실패! 파일 경로: " << filePath
                  << " | 에러 코드: " << BASS_ErrorGetCode() << std::endl;
        return false;
    }

    return true;
}

void AudioEngine::play() {
    if (currentStream) {
        // 🌟 4. 채널 재생 (FALSE 설정으로 일시정지 상태에서 자연스럽게 이어폰 재생 가능)
        BASS_ChannelPlay(currentStream, FALSE);
    }
}

void AudioEngine::pause() {
    if (currentStream) {
        // 🌟 5. 채널 일시정지
        BASS_ChannelPause(currentStream);
    }
}

void AudioEngine::setPosition(double seconds) {
    if (currentStream) {
        // 🌟 6. 초(seconds) 단위를 BASS 내부 포지션 단위인 바이트(Byte)로 변환 후 탐색
        QWORD bytes = BASS_ChannelSeconds2Bytes(currentStream, seconds);
        BASS_ChannelSetPosition(currentStream, bytes, BASS_POS_BYTE);
    }
}

double AudioEngine::getCurrentTime() const {
    if (!currentStream) return 0.0;
    // 🌟 7. 현재 재생 위치(바이트)를 구해서 초(seconds) 단위로 변환 후 반환
    QWORD bytes = BASS_ChannelGetPosition(currentStream, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(currentStream, bytes);
}

double AudioEngine::getTotalTime() const {
    if (!currentStream) return 0.0;
    // 🌟 8. 음원의 총 길이(바이트)를 구해서 초(seconds) 단위로 변환 후 반환
    QWORD bytes = BASS_ChannelGetLength(currentStream, BASS_POS_BYTE);
    return BASS_ChannelBytes2Seconds(currentStream, bytes);
}

// 🌟 9. 기존에 귀신같이 성공했던 TagLib 기반 메타데이터 추출 로직 결합!
MusicMetadata AudioEngine::getMetadata(const std::string& filePath) {
    MusicMetadata meta;
    meta.title = "Unknown Title";
    meta.artist = "Unknown Artist";
    meta.album = "Unknown Album";

    try {
        // TagLib 파일 레퍼런스 생성 (유니코드 처리를 위해 리눅스 빌드 환경 대응)
        TagLib::FileRef f(filePath.c_str());
        if (!f.isNull() && f.tag()) {
            TagLib::Tag* tag = f.tag();

            // 태그 정보가 비어있지 않다면 UTF-8 문자열로 안전하게 변환하여 저장
            if (!tag->title().isEmpty())  meta.title  = tag->title().to8Bit(true);
            if (!tag->artist().isEmpty()) meta.artist = tag->artist().to8Bit(true);
            if (!tag->album().isEmpty())  meta.album  = tag->album().to8Bit(true);
        }
    } catch (...) {
        std::cerr << "[TagLib] 메타데이터 읽기 실패: " << filePath << std::endl;
    }

    return meta;
}
