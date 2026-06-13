#include "MusicPlayerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QIcon>
#include <QString>
#include <QDir>
#include <QStringList>

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Qt Music Player");
    resize(450, 400);

    auto* mainLayout = new QVBoxLayout(this);

    // 🌟 이모지 대신 <img> 태그를 사용해서 아이콘 경로와 크기를 지정!
    lblTitle = new QLabel("<img src='../assets/icons/song.png' width='20' height='20'>   재생 중인 곡 없음", this);
    lblArtist = new QLabel("<img src='../assets/icons/artist.png' width='20' height='20'>   아티스트", this);
    lblAlbum = new QLabel("<img src='../assets/icons/album.png' width='20' height='20'>   앨범", this);

    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblArtist);
    mainLayout->addWidget(lblAlbum);
    mainLayout->addSpacing(15);

    // 🌟 리스트 위젯 추가 (라벨과 버튼 사이에 쏙!)
    playlistWidget = new QListWidget(); // NOLINT
    mainLayout->addWidget(playlistWidget);
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

        // 🌟 QString으로 변환한 다음 HTML 태그 문자열이랑 합쳐주면 끝!
        QString title = QString::fromStdString(meta.title);
        QString artist = QString::fromStdString(meta.artist);
        QString album = QString::fromStdString(meta.album);

        lblTitle->setText("<img src='../assets/icons/song.png' width='20' height='20'> " + title);
        lblArtist->setText("<img src='../assets/icons/artist.png' width='20' height='20'> " + artist);
        lblAlbum->setText("<img src='../assets/icons/album.png' width='20' height='20'> " + album);

        // ==========================================
        // 🌟 리스트에 곡 추가하기!
        QFileInfo fileInfo(fileName); // 파일 경로에서 이름만 쏙 빼주는 도우미

        // 1. 화면에 보여줄 예쁜 이름으로 아이템 만들기
        QListWidgetItem* newItem = new QListWidgetItem(fileInfo.fileName());

        newItem->setIcon(QIcon("assets/icons/music_note.png"));

        // 2. 뒷주머니(UserRole)에 전체 경로 몰래 숨겨두기!
        newItem->setData(Qt::UserRole, fileName);

        // 3. 리스트 위젯에 쏙 넣기
        playlistWidget->addItem(newItem);
        // ==========================================

        sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
        sliderPosition->setValue(0);
        player.play(); // 열면 바로 재생
    }
}

void MusicPlayerWindow::slotPlayListItem(const QListWidgetItem* item) {
    // 🌟 변경 전: item->text() (화면에 보이는 글자)
    // 🌟 변경 후: item->data(Qt::UserRole).toString() (뒷주머니에 숨겨둔 전체 경로!)
    std::string songPath = item->data(Qt::UserRole).toString().toStdString();

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

// 🌟 맨 밑에 폴더 째로 긁어오는 함수 추가!
void MusicPlayerWindow::slotOpenFolder() {
    // 1. 폴더 선택 창 띄우기
    QString dirPath = QFileDialog::getExistingDirectory(this, "음악 폴더 선택", "");

    if (!dirPath.isEmpty()) {
        QDir dir(dirPath);

        // 2. 음악 파일 확장자만 쏙쏙 골라내기 위한 필터 설정
        QStringList filters;
        filters << "*.flac" << "*.mp3" << "*.wav";
        dir.setNameFilters(filters);
        dir.setFilter(QDir::Files | QDir::NoSymLinks); // 파일만 찾기 (폴더 속 폴더 제외)

        // 3. 필터에 걸러진 파일 목록 가져오기
        QFileInfoList fileList = dir.entryInfoList();

        // 4. 찾은 파일들을 하나씩 리스트에 팍팍 집어넣기!
        for (const QFileInfo& fileInfo : fileList) {
            QListWidgetItem* newItem = new QListWidgetItem(fileInfo.fileName());
            newItem->setIcon(QIcon("assets/icons/music_note.png"));

            // 💡 여기서 중요한 건 fileName()이 아니라 absoluteFilePath()로 전체 경로를 숨겨야 해!
            newItem->setData(Qt::UserRole, fileInfo.absoluteFilePath());

            playlistWidget->addItem(newItem);
        }
    }
}