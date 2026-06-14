#include "AudioEngine.h"
#include <iostream>
#include <taglib/fileref.h>  // 🌟 TagLib 메타데이터 파싱용 헤더
#include <taglib/tag.h>

// Album Art
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <QString>

// BASS Plugin
#include "../libs/bass/bassflac.h"
#include "../libs/bass/bassopus.h"

#include <taglib/flacproperties.h>
#include <taglib/wavproperties.h>

#include <taglib/tpropertymap.h> // PropertyMap
#include <QFileInfo>             // fallbackName

AudioEngine::AudioEngine() : currentStream(0) {
    // 🌟 1. 리눅스 사운드 장치 초기화 (기본 출력 장치, 44100Hz 기반 시스템 초기화)
    // 리눅스의 ALSA나 PulseAudio/PipeWire 환경을 BASS가 알아서 매핑합니다.
    if (!BASS_Init(-1, 44100, 0, nullptr, nullptr)) {
        std::cerr << "[BASS Error] 장치 초기화 실패! 에러 코드: " << BASS_ErrorGetCode() << std::endl;
    }

    // FLAC
    if (!BASS_PluginLoad("libbassflac.so", 0)) {
        std::cerr << "[BASS Warning] FLAC 플러그인 로드 실패! 에러 코드: " << BASS_ErrorGetCode() << std::endl;
    }

    // Opus
    if (!BASS_PluginLoad("libbassopus.so", 0)) {
        std::cerr << "[BASS Warning] Opus 플러그인 로드 실패!" << std::endl;
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

MusicMetadata AudioEngine::getMetadata(const std::string& filePath) {
    MusicMetadata meta;
    // 🌟 fallbackName 선언 (제목이 없을 경우 파일 이름으로 대체)
    QString fallbackName = QFileInfo(QString::fromStdString(filePath)).fileName();

    try {
        TagLib::FileRef f(filePath.c_str());
        if (!f.isNull() && f.tag()) {
            meta.title = f.tag()->title().isEmpty() ? fallbackName.toStdString() : f.tag()->title().to8Bit(true);
            meta.artist = f.tag()->artist().isEmpty() ? "Unknown Artist" : f.tag()->artist().to8Bit(true);
            meta.album = f.tag()->album().isEmpty() ? "Unknown Album" : f.tag()->album().to8Bit(true);

            // 🌟 트랙 번호 가져오기
            meta.track = f.tag()->track();

            // 🌟 디스크 번호 가져오기
            if (f.file() && f.file()->properties().contains("DISCNUMBER")) {
                meta.disc = f.file()->properties()["DISCNUMBER"].front().toInt();
            }

            // 오디오 물리 속성 가져오기
            if (f.audioProperties()) {
                meta.bitrate = f.audioProperties()->bitrate();
                meta.sampleRate = f.audioProperties()->sampleRate();
                meta.channels = f.audioProperties()->channels();

                // 비트 심도 추출 (FLAC, WAV 지원)
                if (const auto* flacProps = dynamic_cast<const TagLib::FLAC::Properties*>(f.audioProperties())) {
                    meta.bitDepth = flacProps->bitsPerSample();
                } else if (const auto* wavProps = dynamic_cast<const TagLib::RIFF::WAV::Properties*>(f.audioProperties())) {
                    meta.bitDepth = wavProps->bitsPerSample();
                } else {
                    meta.bitDepth = 0;
                }
            }
        } else {
            // 태그가 아예 없는 파일인 경우
            meta.title = fallbackName.toStdString();
            meta.artist = "Unknown Artist";
            meta.album = "Unknown Album";
        }
    } catch (...) {
        // 읽기 실패 시 최소한 파일 이름이라도 살림
        meta.title = fallbackName.toStdString();
        meta.artist = "Unknown Artist";
        meta.album = "Unknown Album";
    }

    return meta;
}

// 🌟 BASS 엔진 볼륨 조절 (0.0 = 음소거, 1.0 = 최대)
void AudioEngine::setVolume(float volume) {
    if (currentStream) {
        BASS_ChannelSetAttribute(currentStream, BASS_ATTRIB_VOL, volume);
    }
}

// Album Art
QByteArray AudioEngine::getAlbumArt(const std::string& filePath) {
    QString path = QString::fromStdString(filePath);
    QByteArray imageData;

    if (path.endsWith(".mp3", Qt::CaseInsensitive)) {
        TagLib::MPEG::File file(filePath.c_str());
        if (file.hasID3v2Tag()) {
            TagLib::ID3v2::Tag *tag = file.ID3v2Tag();
            // APIC(Attached Picture) 프레임 찾기
            auto frameList = tag->frameListMap()["APIC"];
            if (!frameList.isEmpty()) {
                auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
                if (frame) {
                    imageData = QByteArray(frame->picture().data(), frame->picture().size());
                }
            }
        }
    } else if (path.endsWith(".flac", Qt::CaseInsensitive)) {
        TagLib::FLAC::File file(filePath.c_str());
        const TagLib::List<TagLib::FLAC::Picture*>& picList = file.pictureList();
        if (!picList.isEmpty()) {
            TagLib::FLAC::Picture *pic = picList.front();
            imageData = QByteArray(pic->data().data(), pic->data().size());
        }
    }

    // OPUS나 OGG는 보통 외부 파일(cover.jpg)을 쓰거나 다른 규격을 쓰므로 일단 생략!
    return imageData;
}
