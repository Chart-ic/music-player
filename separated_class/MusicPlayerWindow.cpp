#include "MusicPlayerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QIcon>
#include <QString>

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Qt Music Player");
    resize(450, 400);

    auto* mainLayout = new QVBoxLayout(this);

    // 기본 곡 로드 (추후 플레이리스트 구현 시 비워둬도 됨)
    std::string songPath = "test.flac";
    player.load(songPath);
    MusicMetadata meta = AudioEngine::getMetadata(songPath);

    lblTitle = new QLabel(QString::fromStdString("🎵 " + meta.title), this);
    lblArtist = new QLabel(QString::fromStdString("👤 " + meta.artist), this);
    lblAlbum = new QLabel(QString::fromStdString("💿 " + meta.album), this);

    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblArtist);
    mainLayout->addWidget(lblAlbum);
    mainLayout->addSpacing(15);

    // 🌟 리스트 위젯 추가 (라벨과 버튼 사이에 쏙!)
    playlistWidget = new QListWidget(); // NOLINT
    mainLayout->addWidget(playlistWidget);
    playlistWidget->addItem("test.flac");
    connect(playlistWidget, &QListWidget::itemDoubleClicked, this, &MusicPlayerWindow::slotPlayListItem);

    auto* buttonLayout = new QHBoxLayout(); // NOLINT

    btnFileOpen = new QPushButton(this);
    btnFileOpen->setIcon(QIcon("assets/icons/file.png"));
    btnFileOpen->setIconSize(QSize(40, 40));
    btnFileOpen->setFixedSize(60, 60);
    connect(btnFileOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFile);

    btnPlay = new QPushButton(this);
    btnPlay->setIcon(QIcon("assets/icons/play.png"));
    btnPlay->setIconSize(QSize(40, 40));
    btnPlay->setFixedSize(60, 60);
    connect(btnPlay, &QPushButton::clicked, this, &MusicPlayerWindow::slotPlay);

    btnPause = new QPushButton(this);
    btnPause->setIcon(QIcon("assets/icons/pause.png"));
    btnPause->setIconSize(QSize(40, 40));
    btnPause->setFixedSize(60, 60);
    connect(btnPause, &QPushButton::clicked, this, &MusicPlayerWindow::slotPause);

    QPushButton* btnExit = new QPushButton("종료", this); // NOLINT
    btnExit->setFixedSize(60, 60);
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);

    buttonLayout->addWidget(btnFileOpen);
    buttonLayout->addWidget(btnPlay);
    buttonLayout->addWidget(btnPause);
    buttonLayout->addWidget(btnExit);

    mainLayout->addLayout(buttonLayout);

    sliderPosition = new QSlider(Qt::Horizontal, this);
    sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
    mainLayout->addWidget(sliderPosition);

    connect(sliderPosition, &QSlider::sliderMoved, this, &MusicPlayerWindow::slotSeek);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MusicPlayerWindow::slotUpdateProgress);
    updateTimer->start(100);
}

MusicPlayerWindow::~MusicPlayerWindow() {
    // Qt 객체 트리(this)가 알아서 UI를 delete 하므로 비워둠
}

// ================== 슬롯 함수들 ==================
void MusicPlayerWindow::slotPlay() { player.play(); }
void MusicPlayerWindow::slotPause() { player.pause(); }
void MusicPlayerWindow::slotSeek(int value) { player.seekTo(static_cast<float>(value)); }

void MusicPlayerWindow::slotUpdateProgress() {
    if (!sliderPosition->isSliderDown()) {
        sliderPosition->setValue(static_cast<int>(player.getCurrentTime()));
    }
}

void MusicPlayerWindow::slotOpenFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "음악 파일 열기", "", "Audio Files (*.flac *.mp3 *.wav);;All Files (*)");

    if (!fileName.isEmpty()) {
        std::string songPath = fileName.toStdString();
        player.load(songPath);
        MusicMetadata meta = AudioEngine::getMetadata(songPath);

        lblTitle->setText(QString::fromStdString("🎵 " + meta.title));
        lblArtist->setText(QString::fromStdString("👤 " + meta.artist));
        lblAlbum->setText(QString::fromStdString("💿 " + meta.album));

        sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
        sliderPosition->setValue(0);
        player.play(); // 열면 바로 재생
    }
}

void MusicPlayerWindow::slotPlayListItem(const QListWidgetItem* item) {
    // 1. 클릭한 아이템의 글자(파일 이름)를 가져와서 string으로 변환
    std::string songPath = item->text().toStdString();

    // 2. 오디오 엔진에 곡 로드하고 메타데이터 빼오기
    player.load(songPath);
    MusicMetadata meta = AudioEngine::getMetadata(songPath);

    // 3. UI 텍스트 업데이트
    lblTitle->setText(QString::fromStdString("🎵 " + meta.title));
    lblArtist->setText(QString::fromStdString("👤 " + meta.artist));
    lblAlbum->setText(QString::fromStdString("💿 " + meta.album));

    // 4. 슬라이더 초기화 및 바로 재생!
    sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
    sliderPosition->setValue(0);
    player.play();
}