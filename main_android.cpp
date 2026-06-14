// main_android.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "separated_class/AudioEngine.h"

int main(int argc, char *argv[]) {
    // 📱 모바일 QML 앱은 QApplication 대신 QGuiApplication을 사용합니다.
    QGuiApplication app(argc, argv);

    // QML 화면을 로드하고 제어하는 엔진 생성
    QQmlApplicationEngine engine;

    // 🤝 [여기가 핵심 브릿지]
    // 나중에 우리가 만들 모바일용 오디오 엔진 오브젝트를 QML 세상에 던져줄 준비를 합니다.
    // engine.rootContext()->setContextProperty("AndroidEngine", &yourAndroidAudioEngine);

    // 렌더링할 메인 QML 파일 경로 지정 (에셋 시스템 활용)
    const QUrl url(u"qrc:/qt/qml/music_player/main.qml"_qs);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}