#include "MusicPlayerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QIcon>
#include <QString>
#include <QDir>
#include <QStringList>
#include <QDirIterator> // 🌟 하위 폴더 탐색기 추가!

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

    // 🌟 정렬용 콤보박스 추가
    comboSort = new QComboBox(this);
    comboSort->addItem("파일명 순");   // index 0
    comboSort->addItem("제목 순");     // index 1
    comboSort->addItem("아티스트 순"); // index 2
    comboSort->addItem("앨범 순");     // index 3
    mainLayout->addWidget(comboSort);

    // 콤보박스 값이 바뀔 때마다 정렬 함수 실행!
    connect(comboSort, &QComboBox::currentIndexChanged, this, &MusicPlayerWindow::slotSortPlaylist);
    // ==========================================

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

    // 🌟 여기에 폴더 열기 버튼 생성 코드 추가!!
    btnFolderOpen = new QPushButton(this);
    btnFolderOpen->setIcon(QIcon("assets/icons/folder.png")); // 폴더 아이콘 경로 (맞춰서 수정해!)
    btnFolderOpen->setIconSize(QSize(40, 40));
    btnFolderOpen->setFixedSize(60, 60);
    connect(btnFolderOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFolder);

    // play
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
    buttonLayout->addWidget(btnFolderOpen);
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
    std::string songPath = item->data(Qt::UserRole).toString().toStdString();

    player.load(songPath);
    MusicMetadata meta = AudioEngine::getMetadata(songPath);

    // 🌟 변경: 이모지를 지우고, slotOpenFile에 썼던 HTML img 태그로 똑같이 교체!
    QString title = QString::fromStdString(meta.title);
    QString artist = QString::fromStdString(meta.artist);
    QString album = QString::fromStdString(meta.album);

    lblTitle->setText("<img src='../assets/icons/song.png' width='20' height='20'> " + title);
    lblArtist->setText("<img src='../assets/icons/artist.png' width='20' height='20'> " + artist);
    lblAlbum->setText("<img src='../assets/icons/album.png' width='20' height='20'> " + album);

    sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
    sliderPosition->setValue(0);
    player.play();
}

// 🌟 맨 밑에 폴더(하위 폴더 포함) 째로 긁어오는 함수!
void MusicPlayerWindow::slotOpenFolder() {
    // 1. 폴더 선택 창 띄우기
    QString dirPath = QFileDialog::getExistingDirectory(this, "음악 폴더 선택", "");

    if (!dirPath.isEmpty()) {
        // 2. 음악 파일 확장자만 쏙쏙 골라내기 위한 필터 설정
        QStringList filters;
        filters << "*.flac" << "*.mp3" << "*.wav";

        // 3. 🌟 QDirIterator 탐색 로봇 출동! (핵심: QDirIterator::Subdirectories)
        // 이 옵션을 넣으면 로봇이 폴더 안의 폴더까지 끝까지 파고들어!
        QDirIterator it(dirPath, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

        // 4. 로봇이 파일을 찾을 때마다(hasNext) 리스트에 팍팍 집어넣기!
        while (it.hasNext()) {
            it.next();
            QFileInfo fileInfo = it.fileInfo();

            // 🌟 이 파일의 메타데이터(태그) 뽑아오기!
            std::string path = fileInfo.absoluteFilePath().toStdString();
            MusicMetadata meta = AudioEngine::getMetadata(path);

            QListWidgetItem* newItem = new QListWidgetItem(fileInfo.fileName());
            newItem->setIcon(QIcon("assets/icons/music_note.png"));

            // 🌟 뒷주머니 칸을 여러 개로 나눠서 데이터 저장!
            newItem->setData(Qt::UserRole, fileInfo.absoluteFilePath()); // 기본 경로
            newItem->setData(Qt::UserRole + 1, QString::fromStdString(meta.title));  // +1: 제목
            newItem->setData(Qt::UserRole + 2, QString::fromStdString(meta.artist)); // +2: 아티스트
            newItem->setData(Qt::UserRole + 3, QString::fromStdString(meta.album));  // +3: 앨범

            playlistWidget->addItem(newItem);
        }
    }
}

// 🌟 맨 밑에 정렬 함수 추가
void MusicPlayerWindow::slotSortPlaylist(int index) {
    // 1. 리스트에 있는 아이템들을 싹 다 뽑아서 벡터(배열)에 담기
    std::vector<QListWidgetItem*> items;
    while (playlistWidget->count() > 0) {
        items.push_back(playlistWidget->takeItem(0));
    }

    // 2. 선택한 인덱스(0:파일명, 1:제목, 2:아티스트, 3:앨범)에 맞춰서 정렬!
    std::sort(items.begin(), items.end(), [index](QListWidgetItem* a, QListWidgetItem* b) {
        if (index == 0) {
            return a->text() < b->text(); // 파일명(화면에 보이는 글씨) 기준
        }
        // index 1, 2, 3은 우리가 아까 뒷주머니(UserRole + index)에 넣었던 데이터 기준!
        int targetRole = Qt::UserRole + index;
        return a->data(targetRole).toString() < b->data(targetRole).toString();
    });

    // 3. 예쁘게 정렬된 아이템들을 다시 리스트에 쏙쏙 꽂아 넣기
    for (auto* item : items) {
        playlistWidget->addItem(item);
    }
}