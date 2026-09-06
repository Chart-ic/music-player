#include <algorithm>
#include <functional>

#include "MusicPlayerWindow.h"

#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QFileInfo>
#include <QThread>     // background service
#include <QMetaObject> // 스레드 간 안전한 통신(보고)용
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDirIterator>
#include <QStyle>       // 슬라이더 클릭 위치 계산용
#include <QMouseEvent>  // 마우스 클릭 이벤트 감지용
#include <QRandomGenerator> // 셔플 기능을 위한 랜덤 엔진
#include <QSplitter> // 파일 상단에 추가!
#include <QShortcut>
#include <QDateTime>
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSystemTrayIcon>

namespace {

QString formatDuration(int seconds) {
    if (seconds < 0) {
        return "–:––";
    }

    return QString("%1:%2")
        .arg(seconds / 60)
        .arg(seconds % 60, 2, 10, QChar('0'));
}

QString formatAudioSpec(const MusicMetadata& meta) {
    const QString channelText = (meta.channels == 2)
        ? "Stereo"
        : (meta.channels == 1 ? "Mono" : QString::number(meta.channels) + " Ch");
    const QString bitDepthText = (meta.bitDepth > 0) ? QString("%1-bit | ").arg(meta.bitDepth) : "";

    return QString("♫ %1%2 kHz | %3 kbps | %4")
        .arg(bitDepthText)
        .arg(meta.sampleRate / 1000.0, 0, 'f', 1)
        .arg(meta.bitrate)
        .arg(channelText);
}

} // namespace


QString MusicPlayerWindow::customFontFamily = "";

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("PSMP - Personal Simple Music Player");
    resize(1160, 700);
    setMinimumSize(900, 600);

    setupUI();
    loadSettings();
    setupTrayIcon();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MusicPlayerWindow::slotUpdateProgress);
    updateTimer->start(100);

    loadPlaylist();
}

void MusicPlayerWindow::setupTrayIcon() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    QIcon appIcon = QIcon::fromTheme("media-playback-start");
    if (appIcon.isNull()) {
        appIcon = style()->standardIcon(QStyle::SP_MediaPlay);
    }

    setWindowIcon(appIcon);
    trayIcon = new QSystemTrayIcon(appIcon, this);
    trayIcon->setToolTip("PSMP - Personal Simple Music Player");

    auto* trayMenu = new QMenu(this);
    QAction* restoreAction = trayMenu->addAction("창 열기");
    QAction* playPauseAction = trayMenu->addAction("재생 / 일시정지");
    trayMenu->addSeparator();
    QAction* quitAction = trayMenu->addAction("종료");

    connect(restoreAction, &QAction::triggered, this, &MusicPlayerWindow::restoreWindow);
    connect(playPauseAction, &QAction::triggered, this, &MusicPlayerWindow::slotPlayPause);
    connect(quitAction, &QAction::triggered, this, [this]() {
        isQuitting = true;
        savePlaylist();
        saveSettings();
        trayIcon->hide();
        QApplication::quit();
    });
    connect(trayIcon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                    if (isVisible()) {
                        hide();
                    } else {
                        restoreWindow();
                    }
                }
            });

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void MusicPlayerWindow::restoreWindow() {
    showNormal();
    raise();
    activateWindow();
}

void MusicPlayerWindow::setupUI() {
    setObjectName("musicPlayer");

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);

    auto* leftWidget = new QWidget(this);
    leftWidget->setObjectName("nowPlayingPanel");
    leftWidget->setMinimumWidth(330);
    leftWidget->setMaximumWidth(400);

    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(24, 20, 24, 18);
    leftLayout->setSpacing(4);
    leftWidget->installEventFilter(this);

    auto* nowPlayingLabel = new QLabel("NOW PLAYING", leftWidget);
    nowPlayingLabel->setObjectName("sectionLabel");
    leftLayout->addWidget(nowPlayingLabel);

    lblAlbumArt = new QLabel(leftWidget);
    lblAlbumArt->setObjectName("albumArt");
    lblAlbumArt->setFixedSize(250, 250);
    lblAlbumArt->setScaledContents(false);
    lblAlbumArt->setAlignment(Qt::AlignCenter);
    lblAlbumArt->setText("♫\nNO ARTWORK");

    leftLayout->addWidget(lblAlbumArt, 0, Qt::AlignHCenter);
    leftLayout->addSpacing(6);

    auto* trackInfoWidget = new QWidget(leftWidget);
    trackInfoWidget->setObjectName("trackInfo");
    trackInfoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    lblTitle = new MarqueeLabel(trackInfoWidget);
    lblTitle->setObjectName("trackTitle");
    lblTitle->setText("재생 중인 곡 없음");
    lblTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblTitle->setFixedHeight(29);

    lblArtist = new QLabel("아티스트 정보 없음", trackInfoWidget);
    lblArtist->setObjectName("trackArtist");
    lblArtist->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblArtist->setFixedHeight(18);

    lblAlbum = new QLabel("앨범 정보 없음", trackInfoWidget);
    lblAlbum->setObjectName("trackAlbum");
    lblAlbum->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblAlbum->setFixedHeight(18);

    lblSpecs = new QLabel("Audio Spec: -", leftWidget);
    lblSpecs->setObjectName("trackSpecs");
    lblSpecs->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* trackInfoLayout = new QVBoxLayout(trackInfoWidget);
    trackInfoLayout->setContentsMargins(0, 0, 0, 0);
    trackInfoLayout->setSpacing(0);
    trackInfoLayout->addWidget(lblTitle);
    trackInfoLayout->addWidget(lblArtist);
    trackInfoLayout->addWidget(lblAlbum);

    leftLayout->addWidget(trackInfoWidget);
    leftLayout->addSpacing(2);
    leftLayout->addWidget(lblSpecs);
    leftLayout->addStretch();

    auto* timeLayout = new QHBoxLayout();
    timeLayout->setContentsMargins(0, 0, 0, 0);
    lblCurrentTime = new QLabel("0:00", leftWidget);
    lblCurrentTime->setObjectName("timeLabel");
    lblTotalTime = new QLabel("–:––", leftWidget);
    lblTotalTime->setObjectName("timeLabel");
    timeLayout->addWidget(lblCurrentTime);
    timeLayout->addStretch();
    timeLayout->addWidget(lblTotalTime);
    leftLayout->addLayout(timeLayout);

    sliderPosition = new QSlider(Qt::Horizontal, leftWidget);
    sliderPosition->setObjectName("positionSlider");
    sliderPosition->setRange(0, 1000);
    sliderPosition->setValue(0);
    sliderPosition->installEventFilter(this);
    leftLayout->addWidget(sliderPosition);

    auto* btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 6, 0, 4);
    btnLayout->setSpacing(8);

    btnShuffle = new QPushButton("⤨", leftWidget);
    btnPrev = new QPushButton("‹‹", leftWidget);
    btnPlayPause = new QPushButton("▶", leftWidget);
    btnNext = new QPushButton("››", leftWidget);
    btnRepeat = new QPushButton("↻", leftWidget);

    btnShuffle->setObjectName("controlButton");
    btnPrev->setObjectName("controlButton");
    btnPlayPause->setObjectName("playButton");
    btnNext->setObjectName("controlButton");
    btnRepeat->setObjectName("controlButton");
    btnShuffle->setToolTip("셔플");
    btnPrev->setToolTip("이전 곡");
    btnPlayPause->setToolTip("재생 / 일시정지");
    btnNext->setToolTip("다음 곡");
    btnRepeat->setToolTip("한 곡 반복");
    btnShuffle->setFixedSize(40, 40);
    btnPrev->setFixedSize(40, 40);
    btnPlayPause->setFixedSize(52, 52);
    btnNext->setFixedSize(40, 40);
    btnRepeat->setFixedSize(40, 40);

    btnLayout->addStretch();
    btnLayout->addWidget(btnShuffle);
    btnLayout->addWidget(btnPrev);
    btnLayout->addWidget(btnPlayPause);
    btnLayout->addWidget(btnNext);
    btnLayout->addWidget(btnRepeat);
    btnLayout->addStretch();

    leftLayout->addLayout(btnLayout);

    auto* volLayout = new QHBoxLayout();
    volLayout->setContentsMargins(0, 2, 0, 4);
    auto* lblVolIcon = new QLabel("VOLUME", leftWidget);
    lblVolIcon->setObjectName("volumeLabel");
    sliderVolume = new QSlider(Qt::Horizontal, leftWidget);
    sliderVolume->setObjectName("volumeSlider");
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(80);

    volLayout->addWidget(lblVolIcon);
    volLayout->addWidget(sliderVolume);
    leftLayout->addLayout(volLayout);

    auto* collectionLabel = new QLabel("COLLECTION", leftWidget);
    collectionLabel->setObjectName("sectionLabel");
    leftLayout->addWidget(collectionLabel);

    auto* openLayout = new QHBoxLayout();
    openLayout->setContentsMargins(0, 0, 0, 0);
    openLayout->setSpacing(8);
    btnFileOpen = new QPushButton("파일 열기", leftWidget);
    btnFolderOpen = new QPushButton("폴더 열기", leftWidget);
    btnFileOpen->setObjectName("libraryButton");
    btnFolderOpen->setObjectName("libraryButton");

    openLayout->addWidget(btnFileOpen);
    openLayout->addWidget(btnFolderOpen);
    leftLayout->addLayout(openLayout);

    auto* rightWidget = new QWidget(this);
    rightWidget->setObjectName("libraryPanel");
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(32, 30, 32, 28);
    rightLayout->setSpacing(20);

    auto* libraryHeader = new QHBoxLayout();
    libraryHeader->setContentsMargins(0, 0, 0, 0);
    auto* libraryTitleLayout = new QVBoxLayout();
    libraryTitleLayout->setSpacing(3);
    auto* libraryTitle = new QLabel("내 라이브러리", rightWidget);
    libraryTitle->setObjectName("libraryTitle");
    auto* librarySubtitle = new QLabel("곡을 더블클릭하여 바로 재생하세요", rightWidget);
    librarySubtitle->setObjectName("librarySubtitle");
    libraryTitleLayout->addWidget(libraryTitle);
    libraryTitleLayout->addWidget(librarySubtitle);
    lblPlaylistCount = new QLabel("0곡", rightWidget);
    lblPlaylistCount->setObjectName("playlistCount");
    libraryHeader->addLayout(libraryTitleLayout);
    libraryHeader->addStretch();
    libraryHeader->addWidget(lblPlaylistCount, 0, Qt::AlignVCenter);
    rightLayout->addLayout(libraryHeader);

    playlistTable = new QTableWidget(rightWidget);
    playlistTable->setObjectName("playlistTable");
    playlistTable->setColumnCount(5);
    playlistTable->setHorizontalHeaderLabels({"제목", "아티스트", "앨범", "재생시간", "경로"});
    playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistTable->setContextMenuPolicy(Qt::CustomContextMenu);
    playlistTable->setAlternatingRowColors(true);
    playlistTable->setShowGrid(false);
    playlistTable->setCornerButtonEnabled(false);
    playlistTable->verticalHeader()->setVisible(false);
    playlistTable->verticalHeader()->setDefaultSectionSize(54);
    playlistTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    playlistTable->horizontalHeader()->setStretchLastSection(false);
    playlistTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    playlistTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    playlistTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    playlistTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    playlistTable->setColumnWidth(1, 170);
    playlistTable->setColumnWidth(2, 210);
    playlistTable->setColumnWidth(3, 74);
    playlistTable->setColumnHidden(4, true);

    rightLayout->addWidget(playlistTable);

    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    connect(btnPlayPause, &QPushButton::clicked, this, &MusicPlayerWindow::slotPlayPause);
    connect(btnPrev, &QPushButton::clicked, this, &MusicPlayerWindow::slotPrev);
    connect(btnNext, &QPushButton::clicked, this, &MusicPlayerWindow::slotNext);
    connect(btnFileOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFile);
    connect(btnFolderOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFolder);

    connect(sliderPosition, &QSlider::sliderReleased, this, &MusicPlayerWindow::slotSeek);
    connect(sliderVolume, &QSlider::valueChanged, this, &MusicPlayerWindow::slotVolumeChanged);

    connect(playlistTable, &QTableWidget::cellDoubleClicked, this, &MusicPlayerWindow::slotPlayTableItem);
    connect(playlistTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &MusicPlayerWindow::slotSortTable);
    connect(playlistTable, &QWidget::customContextMenuRequested, this, [this](const QPoint& position) {
        const int row = playlistTable->rowAt(position.y());
        if (row < 0) return;

        QTableWidgetItem* clickedItem = playlistTable->item(row, 0);
        if (!clickedItem || !clickedItem->isSelected()) {
            playlistTable->selectRow(row);
        }
        QMenu contextMenu(this);
        QAction* removeAction = contextMenu.addAction("라이브러리에서 제거");
        if (contextMenu.exec(playlistTable->viewport()->mapToGlobal(position)) == removeAction) {
            slotRemoveSelectedSong();
        }
    });

    auto* removeShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), playlistTable);
    removeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(removeShortcut, &QShortcut::activated, this, &MusicPlayerWindow::slotRemoveSelectedSong);

    connect(btnShuffle, &QPushButton::clicked, this, [this]() {
        isShuffle = !isShuffle;
        btnShuffle->setStyleSheet(isShuffle ? "background-color: #3daee9; color: #ffffff;" : "");
    });

    connect(btnRepeat, &QPushButton::clicked, this, [this]() {
        isRepeat = !isRepeat;
        btnRepeat->setStyleSheet(isRepeat ? "background-color: #3daee9; color: #ffffff;" : "");
    });

    auto* spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    spaceShortcut->setAutoRepeat(false);
    connect(spaceShortcut, &QShortcut::activated, this, &MusicPlayerWindow::slotPlayPause);

    this->setStyleSheet(R"(
        QWidget#musicPlayer {
            background: #1b1e20;
            color: #fcfcfc;
        }
        QWidget#nowPlayingPanel {
            background: #232629;
            border-right: 1px solid #31363b;
        }
        QWidget#libraryPanel {
            background: #1b1e20;
        }
        QLabel#sectionLabel {
            color: #3daee9;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 1.8px;
            padding-bottom: 4px;
        }
        QLabel#albumArt {
            background: #2b2b2b;
            border: 1px solid #3b4045;
            border-radius: 14px;
            color: #888888;
            font-size: 16px;
            font-weight: 700;
            letter-spacing: 2px;
        }
        QLabel#trackTitle {
            color: #ffffff;
            font-size: 21px;
            font-weight: 700;
            padding: 0;
        }
        QLabel#trackArtist {
            color: #b3b3b3;
            font-size: 14px;
            font-weight: 600;
        }
        QLabel#trackAlbum {
            color: #888888;
            font-size: 13px;
        }
        QLabel#trackSpecs {
            color: #8bd5f7;
            background: #2a2e32;
            border-radius: 8px;
            font-size: 11px;
            font-weight: 600;
            padding: 6px 8px;
        }
        QLabel#timeLabel {
            color: #b3b3b3;
            font-size: 11px;
            font-weight: 600;
        }
        QLabel#volumeLabel {
            color: #9aa0a6;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 1px;
        }
        QLabel#libraryTitle {
            color: #fcfcfc;
            font-size: 25px;
            font-weight: 700;
        }
        QLabel#librarySubtitle {
            color: #b3b3b3;
            font-size: 13px;
        }
        QLabel#playlistCount {
            background: #2a2e32;
            border: 1px solid #3b4045;
            border-radius: 12px;
            color: #8bd5f7;
            font-size: 12px;
            font-weight: 700;
            padding: 6px 10px;
        }
        QTableWidget#playlistTable {
            background: #232629;
            alternate-background-color: #2a2e32;
            border: 1px solid #31363b;
            border-radius: 12px;
            color: #eff0f1;
            font-size: 13px;
            outline: none;
            selection-background-color: #3daee9;
            selection-color: #ffffff;
        }
        QTableWidget#playlistTable::item {
            border: none;
            border-bottom: 1px solid #31363b;
            padding: 0 12px;
        }
        QTableWidget#playlistTable::item:hover {
            background: #31363b;
        }
        QTableWidget#playlistTable::item:selected {
            background: #3daee9;
            color: #ffffff;
        }
        QHeaderView::section {
            background: #31363b;
            color: #eff0f1;
            padding: 0 12px;
            border: none;
            border-bottom: 1px solid #3b4045;
            font-size: 11px;
            font-weight: 700;
        }
        QPushButton {
            font-family: inherit;
            border: none;
            color: #ffffff;
            font-weight: 600;
        }
        QPushButton#controlButton {
            background: transparent;
            border-radius: 20px;
            color: #b3b3b3;
            font-size: 17px;
        }
        QPushButton#controlButton:hover {
            background: #31363b;
            color: #ffffff;
        }
        QPushButton#playButton {
            background: #3daee9;
            border-radius: 26px;
            color: #ffffff;
            font-size: 18px;
            padding-left: 2px;
        }
        QPushButton#playButton:hover {
            background: #5cc8f7;
        }
        QPushButton#libraryButton {
            background: #31363b;
            border: 1px solid #4a5058;
            border-radius: 8px;
            color: #eff0f1;
            font-size: 12px;
            padding: 9px 6px;
        }
        QPushButton#libraryButton:hover {
            background: #3b4045;
            border-color: #3daee9;
        }
        QSlider::groove:horizontal {
            background: #50575e;
            border-radius: 3px;
            height: 5px;
        }
        QSlider::sub-page:horizontal {
            background: #3daee9;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #eff0f1;
            border: 2px solid #3daee9;
            width: 10px;
            height: 10px;
            margin: -4px 0;
            border-radius: 7px;
        }
    )");
}

// 슬라이더의 빈 공간을 클릭했을 때 그 위치로 뿅! 하고 점프하는 고급 마법
bool MusicPlayerWindow::eventFilter(QObject *obj, QEvent *event) {
    if (lblAlbumArt && obj == lblAlbumArt->parentWidget() && event->type() == QEvent::Resize) {
        updateAlbumArtGeometry();
    }

    if (obj == sliderPosition && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // 클릭한 X 좌표를 슬라이더의 0~100% 비율로 환산해서 위치 계산
            int val = QStyle::sliderValueFromPosition(sliderPosition->minimum(), sliderPosition->maximum(), mouseEvent->pos().x(), sliderPosition->width());
            sliderPosition->setValue(val);
            slotSeek(); // 계산된 위치로 음악 점프!
        }
    }
    return QWidget::eventFilter(obj, event);
}

void MusicPlayerWindow::addSongToTable(const QString& path, const MusicMetadata& meta) const {
    bool wasSortingEnabled = playlistTable->isSortingEnabled();
    playlistTable->setSortingEnabled(false);

    int row = playlistTable->rowCount();
    playlistTable->insertRow(row);

    auto* itemTitle = new QTableWidgetItem(QString::fromStdString(meta.title)); // NOLINT
    itemTitle->setData(Qt::UserRole, path);
    itemTitle->setData(Qt::UserRole + 1, meta.track);
    itemTitle->setData(Qt::UserRole + 2, meta.disc);

    itemTitle->setData(Qt::UserRole + 3, formatAudioSpec(meta));

    playlistTable->setItem(row, 0, itemTitle);
    playlistTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(meta.artist))); // NOLINT
    playlistTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(meta.album))); // NOLINT

    auto* itemDuration = new QTableWidgetItem(formatDuration(meta.duration)); // NOLINT
    itemDuration->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    itemDuration->setData(Qt::UserRole, meta.duration);
    playlistTable->setItem(row, 3, itemDuration);

    playlistTable->setSortingEnabled(wasSortingEnabled);
    updatePlaylistSummary();
}

void MusicPlayerWindow::updatePlaylistSummary() const {
    const int count = playlistTable->rowCount();
    lblPlaylistCount->setText(QString("%1곡").arg(count));
}

void MusicPlayerWindow::playSongFromTable(int row) {
    if (row < 0 || row >= playlistTable->rowCount()) return;
    currentRow = row;

    QTableWidgetItem* item = playlistTable->item(row, 0);
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    if (player.load(path.toStdString())) {
        lblTitle->setText(item->text());
        lblArtist->setText(playlistTable->item(row, 1)->text());
        lblAlbum->setText(playlistTable->item(row, 2)->text());

        const MusicMetadata metadata = AudioEngine::getMetadata(path.toStdString());
        const QString liveSpec = formatAudioSpec(metadata);
        item->setData(Qt::UserRole + 3, liveSpec);
        lblSpecs->setText(liveSpec);

        // ----------------------------------------------------
        // 🌟 [새로운 코드] 앨범 아트 불러오기 부분
        // ----------------------------------------------------
        QByteArray artData = AudioEngine::getAlbumArt(path.toStdString());

        if (!artData.isEmpty()) {
            QPixmap pixmap;
            pixmap.loadFromData(artData);

            // 1. 화면에 바로 띄우지 않고, 원본 변수에 '저장'만 해둡니다.
            originalAlbumArt = pixmap;
        } else {
            // 1-1. 앨범 아트가 없을 때는 원본 변수를 비워줍니다.
            originalAlbumArt = QPixmap();

            lblAlbumArt->clear();
            lblAlbumArt->setText("No Cover Art");
        }

        // 2. 저장된 원본을 현재 창 크기에 맞게 리사이즈해서 띄워주는 함수 호출!
        updateAlbumArtDisplay();

        playlistTable->selectRow(row);
        sliderPosition->setRange(0, static_cast<int>(player.getTotalTime()));
        lblCurrentTime->setText("0:00");
        lblTotalTime->setText(formatDuration(static_cast<int>(player.getTotalTime())));
        player.setVolume(static_cast<float>(sliderVolume->value()) / 100.0f);

        // 재생 처리 및 버튼 상태 변경
        player.play();
        btnPlayPause->setText("⏸");
        isPlaying = true;
    }
}

// 재생/일시정지 통합 로직
void MusicPlayerWindow::slotPlayPause() {
    // [추가] 300ms(0.3초) 쿨타임 설정
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - lastPlayPauseTime < 300) { // 300 밀리초(0.3초) 이내면
        return; // 아무것도 안 하고 그냥 무시! (연타 방지)
    }
    lastPlayPauseTime = currentTime; // 쿨타임이 지났으면 현재 시간으로 갱신

    if (isPlaying) {
        player.pause();
        btnPlayPause->setText("▶");
        isPlaying = false;
    } else {
        // 이미 곡이 세팅되어 있으면 마저 재생
        if (currentRow >= 0) {
            player.play();
            btnPlayPause->setText("⏸");
            isPlaying = true;
        }
        // 선택된 게 없는데 리스트에 곡이 있으면 맨 첫 곡 재생
        else if (playlistTable->rowCount() > 0) {
            playSongFromTable(0);
        }
    }
}

void MusicPlayerWindow::slotPlayTableItem(int row, int /*column*/) {
    playSongFromTable(row);
}

// 다음 곡 로직 (셔플 기능 적용)
void MusicPlayerWindow::slotNext() {
    if (playlistTable->rowCount() == 0) return;

    int nextRow;
    if (isShuffle) {
        // 셔플 켜져 있으면 리스트 개수 안에서 완전 랜덤 픽!
        nextRow = QRandomGenerator::global()->bounded(playlistTable->rowCount());
    } else {
        // 꺼져 있으면 원래대로 다음 곡 (마지막 곡이면 처음으로)
        nextRow = (currentRow + 1) % playlistTable->rowCount();
    }

    playSongFromTable(nextRow);
}
void MusicPlayerWindow::slotPrev() {
    if (playlistTable->rowCount() == 0) return;
    int prevRow = currentRow - 1;
    if (prevRow < 0) prevRow = playlistTable->rowCount() - 1;
    playSongFromTable(prevRow);
}

void MusicPlayerWindow::slotRemoveSelectedSong() {
    const QModelIndexList selectedIndexes = playlistTable->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) return;

    std::vector<int> rows;
    rows.reserve(selectedIndexes.size());
    for (const QModelIndex& index : selectedIndexes) {
        rows.push_back(index.row());
    }
    std::ranges::sort(rows, std::greater{});

    const bool removesCurrentSong = std::ranges::find(rows, currentRow) != rows.end();
    const int removedBeforeCurrent = static_cast<int>(std::ranges::count_if(rows, [this](int row) {
        return row < currentRow;
    }));

    QString confirmationText;
    if (rows.size() == 1) {
        QTableWidgetItem* titleItem = playlistTable->item(rows.front(), 0);
        const QString title = titleItem ? titleItem->text() : "선택한 곡";
        confirmationText = QString("'%1'을(를) 라이브러리에서 제거할까요?").arg(title);
    } else {
        confirmationText = QString("선택한 %1곡을 라이브러리에서 제거할까요?").arg(rows.size());
    }

    QMessageBox confirmation(this);
    confirmation.setIcon(QMessageBox::Question);
    confirmation.setWindowTitle("라이브러리에서 제거");
    confirmation.setText(confirmationText + "\n원본 음악 파일은 삭제되지 않습니다.");
    QPushButton* removeButton = confirmation.addButton("라이브러리에서 제거", QMessageBox::DestructiveRole);
    confirmation.addButton(QMessageBox::Cancel);
    confirmation.exec();
    if (confirmation.clickedButton() != removeButton) return;

    if (removesCurrentSong) {
        resetNowPlaying();
    } else {
        currentRow -= removedBeforeCurrent;
    }

    for (const int row : rows) {
        playlistTable->removeRow(row);
    }
    updatePlaylistSummary();
    savePlaylist();
}

void MusicPlayerWindow::resetNowPlaying() {
    player.stop();
    currentRow = -1;
    isPlaying = false;
    originalAlbumArt = QPixmap();

    lblAlbumArt->clear();
    lblAlbumArt->setText("♫\nNO ARTWORK");
    lblTitle->setText("재생 중인 곡 없음");
    lblArtist->setText("아티스트 정보 없음");
    lblAlbum->setText("앨범 정보 없음");
    lblSpecs->setText("Audio Spec: -");
    lblCurrentTime->setText("0:00");
    lblTotalTime->setText("–:––");
    sliderPosition->setRange(0, 1000);
    sliderPosition->setValue(0);
    btnPlayPause->setText("▶");
}

void MusicPlayerWindow::slotUpdateProgress() {
    double total = player.getTotalTime();
    double current = player.getCurrentTime();

    // 재생 대상이 없거나 길이가 0이면 진행하지 않음
    if (currentRow < 0 || total <= 0) return;

    lblCurrentTime->setText(formatDuration(static_cast<int>(current)));
    lblTotalTime->setText(formatDuration(static_cast<int>(total)));

    // 1. [가장 중요] 곡이 끝났는지 먼저 체크! (자동 다음 곡 / 반복 재생)
    if (currentRow >= 0 && current >= total - 0.2) {
        if (isRepeat) {
            // 한 곡 반복 재생
            player.setPosition(0);
            player.play();
        } else {
            // 다음 곡 자동 재생 (셔플 옵션도 slotNext 내부에서 알아서 처리됨!)
            slotNext();
        }
        return;
    }

    // 2. 사용자가 마우스로 슬라이더를 잡고 '드래그 중'이 아닐 때만 재생 바 위치 업데이트
    if (!sliderPosition->isSliderDown()) {
        sliderPosition->setValue(static_cast<int>(current));
    }
}

void MusicPlayerWindow::slotVolumeChanged(int value) {
    float vol = static_cast<float>(value) / 100.0f;
    player.setVolume(vol);
    saveSettings();
}

void MusicPlayerWindow::loadSettings() {
    QSettings settings("PSMP", "PersonalSimpleMusicPlayer");
    const int savedVolume = settings.value("playback/volume", 80).toInt();
    sliderVolume->setValue(qBound(0, savedVolume, 100));
}

void MusicPlayerWindow::saveSettings() const {
    QSettings settings("PSMP", "PersonalSimpleMusicPlayer");
    settings.setValue("playback/volume", sliderVolume->value());
}

void MusicPlayerWindow::slotOpenFile() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Music", "", "Audio (*.flac *.mp3 *.opus *.wav *.ogg *.)");
    if (!fileName.isEmpty()) {
        MusicMetadata meta = AudioEngine::getMetadata(fileName.toStdString());
        addSongToTable(fileName, meta);
    }
}

void MusicPlayerWindow::slotOpenFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "Open Folder", "");
    if (dirPath.isEmpty()) return;

    // 버튼 텍스트를 바꿔서 로딩 중임을 알림 (UI 응답성)
    btnFolderOpen->setText("로딩 중...");
    btnFolderOpen->setEnabled(false);

    // 일꾼 스레드(알바생) 하나 고용해서 백그라운드로 보냄!
    QThread::create([this, dirPath]() {
        QDirIterator it(dirPath, {"*.flac", "*.mp3", "*.opus", "*.wav", "*.ogg"}, QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString path = it.next();
            // 디스크를 긁는 무거운 작업은 백그라운드에서 진행
            MusicMetadata meta = AudioEngine::getMetadata(path.toStdString());

            // 분석이 끝나면 메인 스레드(사장)한테 "표에 추가해주세요!" 라고 안전하게 결재 올림
            QMetaObject::invokeMethod(this, [this, path, meta]() {
                addSongToTable(path, meta);
            });
        }

        // 모든 파일 로딩이 끝나면 버튼 상태 원래대로 복구 결재!
        QMetaObject::invokeMethod(this, [this]() {
            btnFolderOpen->setText("폴더 추가");
            btnFolderOpen->setEnabled(true);
        });

    })->start(); // 일꾼 바로 출발!
}

// 이동할 때 한 번에 싹 이동하게 수정
void MusicPlayerWindow::slotSeek() {
    player.setPosition(static_cast<float>(sliderPosition->value()));
}

void MusicPlayerWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon && trayIcon->isVisible() && !isQuitting) {
        hide();
        event->ignore();

        if (!trayHintShown) {
            trayIcon->showMessage(
                "PSMP는 계속 실행 중입니다",
                "트레이 아이콘을 클릭하거나 메뉴에서 창을 다시 열 수 있습니다.",
                QSystemTrayIcon::Information,
                3000
            );
            trayHintShown = true;
        }
        return;
    }

    savePlaylist();
    saveSettings();
    QWidget::closeEvent(event);
}

// 앱을 다시 열 때 필요한 파일 경로만 저장한다. 나머지 메타데이터는 매 실행 시 다시 분석한다.
void MusicPlayerWindow::savePlaylist() const {
    QJsonArray playlistArray;
    for (int row = 0; row < playlistTable->rowCount(); ++row) {
        QTableWidgetItem* item = playlistTable->item(row, 0);
        if (item) {
            QJsonObject songObj;
            songObj["path"] = item->data(Qt::UserRole).toString();
            playlistArray.append(songObj);
        }
    }

    QJsonDocument doc(playlistArray);
    QFile file("playlist.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

// 저장된 경로를 바탕으로 현재 파일의 메타데이터를 다시 분석한다.
void MusicPlayerWindow::loadPlaylist() const {
    QFile file("playlist.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    for (const QJsonValueRef& value : doc.array()) {
        QJsonObject obj = value.toObject();
        QString path = obj["path"].toString();

        QFileInfo fileInfo(path);
        if (!fileInfo.exists()) continue;

        const MusicMetadata metadata = AudioEngine::getMetadata(path.toStdString());
        addSongToTable(path, metadata);
    }

    updatePlaylistSummary();
}

// 우리가 직접 만든 "초지능 다중 정렬 로직"
void MusicPlayerWindow::slotSortTable(int column) {
    if (playlistTable->rowCount() == 0) return;

    // 1. 오름차순/내림차순 토글 결정
    if (currentSortColumn == column) {
        currentSortOrder = (currentSortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        currentSortColumn = column;
        currentSortOrder = Qt::AscendingOrder;
    }

    // 2. 임시 구조체에 모든 행의 아이템들을 뽑아서 담기
    struct RowItems {
        QTableWidgetItem* titleItem;
        QTableWidgetItem* artistItem;
        QTableWidgetItem* albumItem;
        QTableWidgetItem* durationItem;
        bool isPlaying; // 정렬 후에도 현재 재생 곡을 잃어버리지 않기 위해 기억!
    };

    std::vector<RowItems> rows;
    rows.reserve(playlistTable->rowCount());

    for (int r = 0; r < playlistTable->rowCount(); ++r) {
        rows.push_back({
            playlistTable->takeItem(r, 0), // takeItem: 테이블에서 뽑아옴 (삭제 안 됨)
            playlistTable->takeItem(r, 1),
            playlistTable->takeItem(r, 2),
            playlistTable->takeItem(r, 3),
            (r == currentRow)
        });
    }

    // 3. 대망의 다중 정렬 알고리즘
    std::ranges::sort(rows.begin(), rows.end(), [column, this](const RowItems& a, const RowItems& b) {
        QString titleA = a.titleItem->text(), titleB = b.titleItem->text();
        QString artistA = a.artistItem->text(), artistB = b.artistItem->text();
        QString albumA = a.albumItem->text(), albumB = b.albumItem->text();

        // 아까 몰래 숨겨둔 트랙과 디스크 번호 꺼내기
        int trackA = a.titleItem->data(Qt::UserRole + 1).toInt();
        int trackB = b.titleItem->data(Qt::UserRole + 1).toInt();
        int discA = a.titleItem->data(Qt::UserRole + 2).toInt();
        int discB = b.titleItem->data(Qt::UserRole + 2).toInt();

        bool less = false;

        if (column == 1) { // 아티스트 클릭 시: 아티스트 > 앨범 > 디스크 > 트랙 > 제목
            if (artistA != artistB) less = artistA < artistB;
            else if (albumA != albumB) less = albumA < albumB;
            else if (discA != discB) less = discA < discB;
            else if (trackA != trackB) less = trackA < trackB;
            else less = titleA < titleB;
        } else if (column == 2) { // 앨범 클릭 시: 앨범 > 디스크 > 트랙 > 제목
            if (albumA != albumB) less = albumA < albumB;
            else if (discA != discB) less = discA < discB;
            else if (trackA != trackB) less = trackA < trackB;
            else less = titleA < titleB;
        } else { // 제목 클릭 시: 제목 > 아티스트 > 앨범
            if (titleA != titleB) less = titleA < titleB;
            else if (artistA != artistB) less = artistA < artistB;
            else less = albumA < albumB;
        }

        return currentSortOrder == Qt::AscendingOrder ? less : !less;
    });

    // 4. 정렬된 순서대로 다시 테이블에 꽂아넣기
    playlistTable->setRowCount(0);
    // 여기 rows.size() 에도 static_cast<int> 를 씌워줘야 해!
    playlistTable->setRowCount(static_cast<int>(rows.size()));
    currentRow = -1;

    for (int r = 0; r < static_cast<int>(rows.size()); ++r) {
        playlistTable->setItem(r, 0, rows[r].titleItem);
        playlistTable->setItem(r, 1, rows[r].artistItem);
        playlistTable->setItem(r, 2, rows[r].albumItem);
        playlistTable->setItem(r, 3, rows[r].durationItem);

        // 곡 재생 중에 정렬을 바꿨다면, 새로운 줄 번호로 업데이트하고 다시 하이라이트!
        if (rows[r].isPlaying) {
            currentRow = r;
            playlistTable->selectRow(r);
        }
    }

    // 5. 헤더 UI 화살표 방향 업데이트
    playlistTable->horizontalHeader()->setSortIndicator(column, currentSortOrder);
}

// 앨범 아트를 QLabel 크기에 맞춰 비율을 유지하며 부드럽게 리사이즈
void MusicPlayerWindow::updateAlbumArtDisplay() const {
    if (originalAlbumArt.isNull() || !lblAlbumArt) return;

    // 현재 QLabel의 크기에 맞추되:
    // 1. Qt::KeepAspectRatio -> 이미지 비율 깨짐 방지
    // 2. Qt::SmoothTransformation -> 픽셀 깨짐 방지 (고화질 스케일링)
    QPixmap scaled = originalAlbumArt.scaled(
        lblAlbumArt->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );

    lblAlbumArt->setPixmap(scaled);
}

void MusicPlayerWindow::updateAlbumArtGeometry() const {
    if (!lblAlbumArt || !lblAlbumArt->parentWidget()) return;

    constexpr int panelHorizontalMargins = 48;
    constexpr int minimumCoverSide = 250;
    constexpr int maximumCoverSide = 320;
    const int availableWidth = lblAlbumArt->parentWidget()->width() - panelHorizontalMargins;
    const int coverSide = qBound(minimumCoverSide, availableWidth, maximumCoverSide);

    if (lblAlbumArt->size() == QSize(coverSide, coverSide)) return;

    lblAlbumArt->setFixedSize(coverSide, coverSide);
    updateAlbumArtDisplay();
}

// 창 크기가 바뀔 때마다 앨범 아트도 비율에 맞춰 깔끔하게 재계산
void MusicPlayerWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateAlbumArtGeometry();
    updateAlbumArtDisplay();
}

MusicPlayerWindow::~MusicPlayerWindow() = default;
