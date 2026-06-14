// main.cpp 내부

#include <QApplication>
#include <QFontDatabase>
#include "separated_class/MusicPlayerWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 🌟 1. 얇은 폰트와 굵은 폰트 둘 다 메모리에 로드!
    QFontDatabase::addApplicationFont("assets/fonts/PretendardJP-Thin.ttf"); // 기본용 얇은 폰트
    int boldId = QFontDatabase::addApplicationFont("assets/fonts/PretendardJP-Bold.ttf"); // 강조용 굵은 폰트

    if (boldId != -1) {
        // 패밀리 이름 저장 ("Pretendard" 같은 이름이 들어감)
        MusicPlayerWindow::customFontFamily = QFontDatabase::applicationFontFamilies(boldId).at(0);

        // 🌟 2. 앱 전체 기본 폰트는 'Thin'으로 세팅!
        QFont defaultFont(MusicPlayerWindow::customFontFamily);
        defaultFont.setWeight(QFont::Thin); // Thin이 너무 안 보이면 QFont::Light 나 QFont::Normal 로 바꿔도 됨!
        a.setFont(defaultFont);
    }

    MusicPlayerWindow w;
    w.show();

    return a.exec();
}