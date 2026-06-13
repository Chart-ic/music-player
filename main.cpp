#include <QApplication>

// 🌟 [핵심] 다른 어떤 파일보다 무조건 '먼저' 알맹이를 생성하도록 맨 위로 올림!
#define MINIAUDIO_IMPLEMENTATION
#include "separated_class/AudioEngine.h"

// 💡 이제 여기서 불러오는 MusicPlayerWindow.h는 중복 방지 시스템 덕분에
// AudioEngine.h를 또 부르지 않고 안전하게 넘어감!
#include "separated_class/MusicPlayerWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MusicPlayerWindow window;
    window.show();

    return QApplication::exec();
}