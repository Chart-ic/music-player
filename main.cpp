#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QIcon>
#include <QPixmap>

// 기존 오디오 엔진 연결 (매크로 필수!)
#define MINIAUDIO_IMPLEMENTATION
#include "separated_class/AudioEngine.h"

// 🎵 음악 플레이어 메인 윈도우 클래스
class MusicPlayerWindow : public QWidget {
    Q_OBJECT

private:
    AudioEngine player;
    QTimer* updateTimer;

    QLabel* lblTitle;
    QLabel* lblArtist;
    QLabel* lblAlbum;
    QSlider* sliderPosition;
    QPushButton* btnPlay;
    QPushButton* btnPause;

public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr) : QWidget(parent) {
        // 창 기본 설정
        setWindowTitle("Qt Music Player");
        resize(450, 200);

        auto* mainLayout = new QVBoxLayout(this);

        // 1. 노래 로드 및 메타데이터 추출
        std::string songPath = "test.flac"; // 나중에 파일 열기 창으로 바꾸면 돼!
        player.load(songPath);
        MusicMetadata meta = AudioEngine::getMetadata(songPath);

        // 2. 곡 정보 표시 라벨
        // QString::fromStdString()을 쓰면 C++ string을 Qt용 문자열로 안전하게 변환해 줌
        lblTitle = new QLabel(QString::fromStdString("🎵 " + meta.title), this);
        lblArtist = new QLabel(QString::fromStdString("👤 " + meta.artist), this);
        lblAlbum = new QLabel(QString::fromStdString("💿 " + meta.album), this);

        mainLayout->addWidget(lblTitle);
        mainLayout->addWidget(lblArtist);
        mainLayout->addWidget(lblAlbum);

        mainLayout->addSpacing(15); // 약간의 여백

        // 3. 네가 만든 아이콘이 들어간 재생/일시정지 버튼!
        auto* buttonLayout = new QHBoxLayout();

        btnPlay = new QPushButton(this);
        btnPlay->setIcon(QIcon("assets/icons/play.png")); // 아이콘 바로 로드!
        btnPlay->setIconSize(QSize(40, 40));              // 아이콘 크기
        btnPlay->setFixedSize(60, 60);                    // 버튼 겉면 크기

        btnPause = new QPushButton(this);
        btnPause->setIcon(QIcon("assets/icons/pause.png"));
        btnPause->setIconSize(QSize(40, 40));
        btnPause->setFixedSize(60, 60);

        buttonLayout->addWidget(btnPlay);
        buttonLayout->addWidget(btnPause);
        mainLayout->addLayout(buttonLayout);

        // 4. 슬라이더 (재생 바) 추가
        sliderPosition = new QSlider(Qt::Horizontal, this);
        sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
        mainLayout->addWidget(sliderPosition);

        // 5. 버튼 클릭 및 슬라이더 조작 이벤트 연결 (시그널 & 슬롯)
        connect(btnPlay, &QPushButton::clicked, this, &MusicPlayerWindow::slotPlay);
        connect(btnPause, &QPushButton::clicked, this, &MusicPlayerWindow::slotPause);
        connect(sliderPosition, &QSlider::sliderMoved, this, &MusicPlayerWindow::slotSeek);

        // 6. 재생 바 동기화 타이머 (100ms마다)
        updateTimer = new QTimer(this);
        connect(updateTimer, &QTimer::timeout, this, &MusicPlayerWindow::slotUpdateProgress);
        updateTimer->start(100);
    }

private slots:
    void slotPlay() { player.play(); }
    void slotPause() { player.pause(); }
    void slotSeek(int value) { player.seekTo(static_cast<float>(value)); }

    void slotUpdateProgress() {
        if (!sliderPosition->isSliderDown()) {
            sliderPosition->setValue(static_cast<int>(player.getCurrentTime()));
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MusicPlayerWindow window;
    window.show();
    return QApplication::exec();
}

// 💡 클래스가 main.cpp에 있을 때 꼭 필요한 MOC 처리 구문
#include "main.moc"