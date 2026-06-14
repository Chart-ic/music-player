#include "MusicPlayerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDirIterator>
// 🌟 불필요했던 <QDir> 등 제거 완료

// initial font reset
QString MusicPlayerWindow::customFontFamily = "";

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Modern Hi-Res Player");
    resize(1000, 600);

    setupUI();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MusicPlayerWindow::slotUpdateProgress);
    updateTimer->start(100);

    loadPlaylist();
}

void MusicPlayerWindow::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // --- [좌측 섹션] ---
    // 🌟 Qt가 알아서 지워주는데 Linter가 오해하는 부분에 // NOLINT 방패 장착 (메모리 누수 경고 5개 해결)
    auto* leftLayout = new QVBoxLayout(); // NOLINT
    leftLayout->setContentsMargins(20, 20, 20, 20);

    lblAlbumArt = new QLabel(this);
    lblAlbumArt->setFixedSize(300, 300);
    lblAlbumArt->setStyleSheet("background-color: #333; border-radius: 15px; color: white;");
    lblAlbumArt->setAlignment(Qt::AlignCenter);
    lblAlbumArt->setText("Album Art");


    lblTitle = new QLabel("곡 제목", this);

    // 🌟 제목 라벨만 꺼내서 폰트 크기 키우고 굵기(Weight)를 Bold로 강제 지정!
    QFont titleFont(customFontFamily); // 저장해둔 폰트 이름 꺼내오기
    titleFont.setPointSize(18);
    titleFont.setWeight(QFont::Bold);  // 여기서 Bold 펀치!
    lblTitle->setFont(titleFont);

    lblArtist = new QLabel("아티스트", this);
    // (아티스트나 앨범명은 따로 지정 안 했으니 알아서 기본 얇은 폰트가 적용됨!)

    lblAlbum = new QLabel("앨범명", this);
    lblSpecs = new QLabel("🎵 음원 스펙", this); // 🌟 라벨 생성
    lblSpecs->setStyleSheet("font-size: 11pt; color: #aaaaaa;"); // 🌟 살짝 연한 회색으로 감성 추가

    auto* ctrlLayout = new QHBoxLayout(); // NOLINT
    btnPrev = new QPushButton("⏮", this);
    btnPlay = new QPushButton("▶", this);
    btnPause = new QPushButton("⏸", this);
    btnNext = new QPushButton("⏭", this);

    ctrlLayout->addWidget(btnPrev);
    ctrlLayout->addWidget(btnPlay);
    ctrlLayout->addWidget(btnPause);
    ctrlLayout->addWidget(btnNext);

    sliderPosition = new QSlider(Qt::Horizontal, this);

    auto* volLayout = new QHBoxLayout(); // NOLINT
    auto* lblVolIcon = new QLabel("🔊", this);
    sliderVolume = new QSlider(Qt::Horizontal, this);
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(80);
    volLayout->addWidget(lblVolIcon);
    volLayout->addWidget(sliderVolume);

    leftLayout->addWidget(lblAlbumArt);
    leftLayout->addSpacing(20);
    leftLayout->addWidget(lblTitle);
    leftLayout->addWidget(lblArtist);
    leftLayout->addWidget(lblAlbum);
    leftLayout->addWidget(lblSpecs); // 🌟 앨범 밑에 스펙 라벨 배치!
    leftLayout->addStretch();
    leftLayout->addWidget(sliderPosition);
    leftLayout->addLayout(ctrlLayout);
    leftLayout->addLayout(volLayout);

    // --- [우측 섹션] ---
    auto* rightLayout = new QVBoxLayout(); // NOLINT

    auto* topMenuLayout = new QHBoxLayout(); // NOLINT
    btnFileOpen = new QPushButton("파일 추가", this);
    btnFolderOpen = new QPushButton("폴더 추가", this);
    topMenuLayout->addWidget(btnFileOpen);
    topMenuLayout->addWidget(btnFolderOpen);
    topMenuLayout->addStretch();

    playlistTable = new QTableWidget(0, 3, this);
    playlistTable->setHorizontalHeaderLabels({"제목", "아티스트", "앨범"});
    playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistTable->setAlternatingRowColors(true);
    playlistTable->horizontalHeader()->setStretchLastSection(true);
    playlistTable->verticalHeader()->setVisible(false);
    playlistTable->setSortingEnabled(true);

    rightLayout->addLayout(topMenuLayout);
    rightLayout->addWidget(playlistTable);

    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 3);

    // 이벤트 연결
    connect(btnPlay, &QPushButton::clicked, this, &MusicPlayerWindow::slotPlay);
    connect(btnPause, &QPushButton::clicked, this, &MusicPlayerWindow::slotPause);
    connect(btnFileOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFile);
    connect(btnFolderOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFolder);
    connect(playlistTable, &QTableWidget::cellDoubleClicked, this, &MusicPlayerWindow::slotPlayTableItem);
    connect(sliderPosition, &QSlider::sliderMoved, this, &MusicPlayerWindow::slotSeek);
    connect(sliderVolume, &QSlider::valueChanged, this, &MusicPlayerWindow::slotVolumeChanged);
    connect(btnNext, &QPushButton::clicked, this, &MusicPlayerWindow::slotNext);
    connect(btnPrev, &QPushButton::clicked, this, &MusicPlayerWindow::slotPrev);
}

void MusicPlayerWindow::addSongToTable(const QString& path, const MusicMetadata& meta) const {
    bool wasSortingEnabled = playlistTable->isSortingEnabled();
    playlistTable->setSortingEnabled(false);

    int row = playlistTable->rowCount();
    playlistTable->insertRow(row);

    // 🌟 나머지 메모리 누수 경고 3개도 NOLINT로 Linter 차단!
    auto* itemTitle = new QTableWidgetItem(QString::fromStdString(meta.title)); // NOLINT
    itemTitle->setData(Qt::UserRole, path);

    playlistTable->setItem(row, 0, itemTitle);
    playlistTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(meta.artist))); // NOLINT
    playlistTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(meta.album))); // NOLINT

    playlistTable->setSortingEnabled(wasSortingEnabled);
}

void MusicPlayerWindow::slotPlayTableItem(int row, int /*column*/) {
    QTableWidgetItem* item = playlistTable->item(row, 0);
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    if (player.load(path.toStdString())) {
        lblTitle->setText(item->text());
        lblArtist->setText(playlistTable->item(row, 1)->text());
        lblAlbum->setText(playlistTable->item(row, 2)->text());

        MusicMetadata meta = AudioEngine::getMetadata(path.toStdString());

        // 🌟 채널 및 비트 심도 포맷팅
        QString channelStr = (meta.channels == 2) ? "Stereo" : (meta.channels == 1 ? "Mono" : QString::number(meta.channels) + " Ch");
        QString bitDepthStr = (meta.bitDepth > 0) ? QString(" | %1-bit").arg(meta.bitDepth) : "";

        // 예: "🎵 96.0 kHz | 24-bit | 3000 kbps | Stereo"
        QString specText = QString("%2 | %1 kHz | %3 kbps | %4")
                               .arg(meta.sampleRate / 1000.0, 0, 'f', 1)
                               .arg(bitDepthStr)
                               .arg(meta.bitrate)
                               .arg(channelStr);
        lblSpecs->setText(specText);

        QByteArray artData = AudioEngine::getAlbumArt(path.toStdString());
        if (!artData.isEmpty()) {
            QPixmap pixmap;
            pixmap.loadFromData(artData);
            lblAlbumArt->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            lblAlbumArt->clear();
            lblAlbumArt->setText("No Cover Art");
        }

        sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
        player.setVolume(static_cast<float>(sliderVolume->value()) / 100.0f);
        player.play();
    }
}

void MusicPlayerWindow::slotVolumeChanged(int value) {
    float vol = static_cast<float>(value) / 100.0f;
    player.setVolume(vol);
}

void MusicPlayerWindow::slotOpenFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Music", "", "Audio (*.flac *.mp3 *.opus *.wav *.ogg)");
    if (!fileName.isEmpty()) {
        MusicMetadata meta = AudioEngine::getMetadata(fileName.toStdString());
        addSongToTable(fileName, meta);
    }
}

void MusicPlayerWindow::slotOpenFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "Open Folder", "");
    if (!dirPath.isEmpty()) {
        QDirIterator it(dirPath, {"*.flac", "*.mp3", "*.opus", "*.wav", "*.ogg"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            MusicMetadata meta = AudioEngine::getMetadata(path.toStdString());
            addSongToTable(path, meta);
        }
    }
}

void MusicPlayerWindow::slotPlay() { player.play(); }
void MusicPlayerWindow::slotPause() { player.pause(); }
void MusicPlayerWindow::slotSeek(int value) { player.setPosition(static_cast<double>(value)); }

// 🌟 const 추가된 함수들
void MusicPlayerWindow::slotUpdateProgress() const {
    if (!sliderPosition->isSliderDown()) {
        sliderPosition->setValue(static_cast<int>(player.getCurrentTime()));
    }
}

void MusicPlayerWindow::slotNext() const {
    if (playlistTable->rowCount() > 0) {
        // 3단계 오토파일럿 로직 자리
    }
}

void MusicPlayerWindow::slotPrev() const {
    if (playlistTable->rowCount() > 0) {
        // 3단계 오토파일럿 로직 자리
    }
}

void MusicPlayerWindow::closeEvent(QCloseEvent *event) {
    savePlaylist();
    QWidget::closeEvent(event);
}

void MusicPlayerWindow::savePlaylist() const {
    QJsonArray playlistArray;
    for (int row = 0; row < playlistTable->rowCount(); ++row) {
        QTableWidgetItem* item = playlistTable->item(row, 0);
        if (item) {
            playlistArray.append(item->data(Qt::UserRole).toString());
        }
    }

    QJsonDocument doc(playlistArray);
    QFile file("playlist.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MusicPlayerWindow::loadPlaylist() const {
    QFile file("playlist.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    for (const QJsonValue& value : doc.array()) {
        QString path = value.toString();
        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) continue;

        MusicMetadata meta = AudioEngine::getMetadata(path.toStdString());
        addSongToTable(path, meta);
    }
}

MusicPlayerWindow::~MusicPlayerWindow() {}