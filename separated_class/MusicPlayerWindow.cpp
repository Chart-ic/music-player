#include "MusicPlayerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QIcon>
#include <QString>
#include <QDir>
#include <QStringList>
#include <QDirIterator>  // 🌟 중복 include 정리 완료!
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QFileInfo>
#include <algorithm>     // 🌟 std::sort 사용을 위해 반드시 필요한 헤더 누락 추가!
#include <vector>

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Qt Music Player");
    resize(450, 400);

    auto* mainLayout = new QVBoxLayout(this);

    // 라벨 생성 및 아이콘 매핑
    lblTitle = new QLabel("<img src='../assets/icons/song.png' width='20' height='20'>   재생 중인 곡 없음", this);
    lblArtist = new QLabel("<img src='../assets/icons/artist.png' width='20' height='20'>   아티스트", this);
    lblAlbum = new QLabel("<img src='../assets/icons/album.png' width='20' height='20'>   앨범", this);

    mainLayout->addWidget(lblTitle);
    mainLayout->addWidget(lblArtist);
    mainLayout->addWidget(lblAlbum);
    mainLayout->addSpacing(15);

    // 정렬용 콤보박스
    comboSort = new QComboBox(this);
    comboSort->addItem("파일명 순");
    comboSort->addItem("제목 순");
    comboSort->addItem("아티스트 순");
    comboSort->addItem("앨범 순");
    mainLayout->addWidget(comboSort);

    connect(comboSort, &QComboBox::currentIndexChanged, this, &MusicPlayerWindow::slotSortPlaylist);

    // 플레이리스트 위젯
    playlistWidget = new QListWidget(this);
    mainLayout->addWidget(playlistWidget);
    connect(playlistWidget, &QListWidget::itemDoubleClicked, this, &MusicPlayerWindow::slotPlayListItem);

    // 하단 제어 버튼 레이아웃
    auto* buttonLayout = new QHBoxLayout();

    btnFileOpen = new QPushButton(this);
    btnFileOpen->setIcon(QIcon("assets/icons/file.png"));
    btnFileOpen->setIconSize(QSize(40, 40));
    btnFileOpen->setFixedSize(60, 60);
    connect(btnFileOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFile);

    btnFolderOpen = new QPushButton(this);
    btnFolderOpen->setIcon(QIcon("assets/icons/folder.png"));
    btnFolderOpen->setIconSize(QSize(40, 40));
    btnFolderOpen->setFixedSize(60, 60);
    connect(btnFolderOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFolder);

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

    auto* btnExit = new QPushButton("종료", this);
    btnExit->setFixedSize(60, 60);
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);

    buttonLayout->addWidget(btnFileOpen);
    buttonLayout->addWidget(btnFolderOpen);
    buttonLayout->addWidget(btnPlay);
    buttonLayout->addWidget(btnPause);
    buttonLayout->addWidget(btnExit);
    mainLayout->addLayout(buttonLayout);

    // 프로그레스 슬라이더
    sliderPosition = new QSlider(Qt::Horizontal, this);
    sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
    mainLayout->addWidget(sliderPosition);
    connect(sliderPosition, &QSlider::sliderMoved, this, &MusicPlayerWindow::slotSeek);

    // 진행 상태 업데이트 타이머
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MusicPlayerWindow::slotUpdateProgress);
    updateTimer->start(100);

    // 이전 세션 플레이리스트 불러오기
    loadPlaylist();
}

MusicPlayerWindow::~MusicPlayerWindow() {
    // Qt 부모-자식 구조 기반 자동 해제
}

void MusicPlayerWindow::slotPlay() { player.play(); }
void MusicPlayerWindow::slotPause() { player.pause(); }

void MusicPlayerWindow::slotSeek(int value) {
    // AudioEngine이 분리된 인터페이스 명세(double 혹은 float)에 맞춰 호출
    player.setPosition(static_cast<double>(value));
}

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
        if (player.load(songPath)) {
            MusicMetadata meta = AudioEngine::getMetadata(songPath);

            QString title = QString::fromStdString(meta.title);
            QString artist = QString::fromStdString(meta.artist);
            QString album = QString::fromStdString(meta.album);

            lblTitle->setText("<img src='../assets/icons/song.png' width='20' height='20'> " + title);
            lblArtist->setText("<img src='../assets/icons/artist.png' width='20' height='20'> " + artist);
            lblAlbum->setText("<img src='../assets/icons/album.png' width='20' height='20'> " + album);

            QFileInfo fileInfo(fileName);
            auto* newItem = new QListWidgetItem(fileInfo.fileName());
            newItem->setIcon(QIcon("assets/icons/music_note.png"));

            // 뒷주머니에 데이터 보관
            newItem->setData(Qt::UserRole, fileName);
            newItem->setData(Qt::UserRole + 1, title);
            newItem->setData(Qt::UserRole + 2, artist);
            newItem->setData(Qt::UserRole + 3, album);

            playlistWidget->addItem(newItem);

            sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
            sliderPosition->setValue(0);
            player.play();
        }
    }
}

void MusicPlayerWindow::slotPlayListItem(const QListWidgetItem* item) {
    if (!item) return;
    std::string songPath = item->data(Qt::UserRole).toString().toStdString();

    if (player.load(songPath)) {
        MusicMetadata meta = AudioEngine::getMetadata(songPath);

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
}

void MusicPlayerWindow::slotOpenFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "음악 폴더 선택", "");

    if (!dirPath.isEmpty()) {
        QStringList filters;
        filters << "*.flac" << "*.mp3" << "*.wav";

        QDirIterator it(dirPath, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            it.next();
            QFileInfo fileInfo = it.fileInfo();

            std::string path = fileInfo.absoluteFilePath().toStdString();
            MusicMetadata meta = AudioEngine::getMetadata(path);

            auto* newItem = new QListWidgetItem(fileInfo.fileName());
            newItem->setIcon(QIcon("assets/icons/music_note.png"));

            newItem->setData(Qt::UserRole, fileInfo.absoluteFilePath());
            newItem->setData(Qt::UserRole + 1, QString::fromStdString(meta.title));
            newItem->setData(Qt::UserRole + 2, QString::fromStdString(meta.artist));
            newItem->setData(Qt::UserRole + 3, QString::fromStdString(meta.album));

            playlistWidget->addItem(newItem);
        }
    }
}

void MusicPlayerWindow::slotSortPlaylist(int index) {
    std::vector<QListWidgetItem*> items;
    while (playlistWidget->count() > 0) {
        items.push_back(playlistWidget->takeItem(0));
    }

    std::sort(items.begin(), items.end(), [index](QListWidgetItem* a, QListWidgetItem* b) {
        if (index == 0) {
            return a->text() < b->text();
        }
        int targetRole = Qt::UserRole + index;
        return a->data(targetRole).toString() < b->data(targetRole).toString();
    });

    for (auto* item : items) {
        playlistWidget->addItem(item);
    }
}

void MusicPlayerWindow::closeEvent(QCloseEvent *event) {
    savePlaylist();
    QWidget::closeEvent(event);
}

void MusicPlayerWindow::savePlaylist() {
    QJsonArray playlistArray;

    for (int i = 0; i < playlistWidget->count(); ++i) {
        QListWidgetItem* item = playlistWidget->item(i);
        QString path = item->data(Qt::UserRole).toString();
        playlistArray.append(path);
    }

    QJsonDocument doc(playlistArray);
    QFile file("playlist.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MusicPlayerWindow::loadPlaylist() {
    QFile file("playlist.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    QJsonArray array = doc.array();

    for (const QJsonValue& value : array) {
        QString path = value.toString();
        QFileInfo fileInfo(path);

        if (!fileInfo.exists()) continue;

        std::string stdPath = path.toStdString();
        MusicMetadata meta = AudioEngine::getMetadata(stdPath);

        auto* newItem = new QListWidgetItem(fileInfo.fileName());
        newItem->setIcon(QIcon("assets/icons/music_note.png"));

        newItem->setData(Qt::UserRole, path);
        newItem->setData(Qt::UserRole + 1, QString::fromStdString(meta.title));
        newItem->setData(Qt::UserRole + 2, QString::fromStdString(meta.artist));
        newItem->setData(Qt::UserRole + 3, QString::fromStdString(meta.album));

        playlistWidget->addItem(newItem);
    }
} // 🌟 뒤틀려 삐져나와있던 마지막 중괄호 블록 완벽 정돈!