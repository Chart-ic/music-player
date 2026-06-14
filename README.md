# 🎵 PSMP (Personal Simple Music Player)

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=for-the-badge&logo=c%2B%2B" alt="C++20">
  <img src="https://img.shields.io/badge/Qt-6.x-green?style=for-the-badge&logo=qt" alt="Qt6">
  <img src="https://img.shields.io/badge/Audio%20Engine-BASS-orange?style=for-the-badge" alt="BASS Engine">
  <img src="https://img.shields.io/badge/Platform-Linux%20%7C%20Cross--Platform%20Ready-lightgrey?style=for-the-badge&logo=linux" alt="Linux">
</p>

**PSMP**는 C++20과 Qt 6를 기반으로 개발된 가볍고 강력한 개인용 커스텀 뮤직 플레이어입니다. 상용 플레이어 수준의 고음질 오디오 렌더링과 대용량 라이브러리를 부드럽게 처리하는 비동기 멀티스레드 아키텍처를 자랑합니다.

---

## ✨ Key Features (주요 기능)

- 🎧 **고음질 오디오 엔진 (BASS Engine)**
  - MP3, FLAC, OPUS 등 다양한 고음질 포맷 지원
  - 32-bit / 44.1kHz ~ 192kHz 원음 재생 및 하드웨어 사운드 출력 최적화
- ⚡ **비동기 멀티스레드 라이브러리 스캔 (Background Worker)**
  - 수천 개의 고용량 음원 폴더를 추가해도 UI가 전혀 멈추지 않는(`QThread` 기반) 비동기 메타데이터 파싱
- 📛 **스마트 전광판 라벨 (Marquee Effect)**
  - 곡 제목이 너무 길 경우 레이아웃을 해치지 않고, 좌측으로 부드럽게 스크롤링되는 CPU 최적화 커스텀 마스킹 라벨 탑재
- 🗂️ **스마트 메타 데이터 정렬 (Smart Sorting)**
  - 단순 텍스트 정렬을 넘어 `앨범 ➡️ 디스크 번호 ➡️ 트랙 번호 ➡️ 곡 제목` 순으로 음반 규격을 완벽하게 추적하는 다중 레이어 정렬 시스템
- 🔀 **셔플 & 반복 재생 (Shuffle & Repeat)**
  - 중복이나 누락 없이 완벽하게 무작위로 곡을 섞어주는 인텔리전트 셔플 및 한 곡 반복 모드 지원
- 🌙 **모던 다크 UI (Modern Minimalistic UI)**
  - 프리텐다드(Pretendard) 폰트와 하드웨어 가속(Qt RHI) 기반의 세련되고 눈이 편안한 다크 모드 인터페이스

---

## 🛠️ Tech Stack (기술 스택)

- **Language:** C++20
- **Framework:** Qt 6 (Widgets, RHI 가속)
- **Audio Core:** BASS Audio Library (with FLAC, OPUS Plugins)
- **Metadata Parser:** TagLib
- **Build System:** CMake 3.20+

---

## 🚀 Getting Started (시작하기 - Linux 기준)

### Prerequisites (의존성 설치)
빌드 전 시스템에 Qt6, TagLib 및 필수 도구들이 설치되어 있어야 합니다.
#### Ubuntu / Debian 기준
```
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libtag1-dev
```

Installation & Build (설치 및 빌드)

```
git clone https://github.com/Chart-ic/music-player.git
cd music-player
```

CMake를 통해 프로젝트를 빌드합니다.

```
mkdir build && cd build
cmake ..
make -j$(nproc)
```

생성된 파일을 실행합니다.

```
./music_player
```

# 🗺️ Roadmap (향후 로드맵)
  - [x] C++20 / Qt6 기반 기본 아키텍처 수립
  - [x] 비동기 멀티스레드 메타데이터 로더 구현
  - [x] MarqueeLabel을 이용한 긴 제목 스크롤링 최적화
  - [ ] Vulkan 네이티브 지원: OS별 그래픽 드라이버 오버헤드 최소화 및 CPU 부담 경감 (Qt RHI 인프라 활용)
  - [ ] 크로스 플랫폼 배포 가동: Linux 테스팅 완료 후 Windows, Android, macOS 순차 빌드 자동화 시동
  - [ ] Android 전용 모바일 UI 탑재: 모바일 터치 패러다임에 맞춘 Qt Quick (QML) 레이어 설계

# 📄 License
위에 라이센스 탭을 확인하십시오

# etc..
- 건의/개선 사항 모두 긍정적으로 검토하겠습니당
