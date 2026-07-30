#include <algorithm>

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


QString MusicPlayerWindow::customFontFamily = "";

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("PSMP - Personal Simple Music Player");
    resize(1000, 600);

    setupUI();

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MusicPlayerWindow::slotUpdateProgress);
    updateTimer->start(100);

    loadPlaylist();
}

void MusicPlayerWindow::setupUI() {
    // --------------------------------------------------
    // 메인 레이아웃 및 QSplitter 생성
    // --------------------------------------------------
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    // ==================================================
    // 1. [좌측 패널] - 스포티파이/멜론 스타일 (하단 고정)
    // ==================================================
    auto* leftWidget = new QWidget(this);
    leftWidget->setMinimumWidth(320);
    leftWidget->setMaximumWidth(420);

    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(12);

    // --------------------------------------------------
    // 1-A. [상단 영역] 앨범 아트 & 곡 정보
    // --------------------------------------------------
    lblAlbumArt = new QLabel(leftWidget);
    lblAlbumArt->setMinimumSize(250, 250);
    lblAlbumArt->setMaximumSize(320, 320);
    lblAlbumArt->setScaledContents(false);
    lblAlbumArt->setAlignment(Qt::AlignCenter);
    lblAlbumArt->setText("Album Art");
    lblAlbumArt->setStyleSheet("background-color: #2b2b2b; border-radius: 8px; color: #888888;");

    leftLayout->addWidget(lblAlbumArt, 0, Qt::AlignCenter);

    // [1-2] 곡 정보 (제목, 아티스트, 앨범, 스펙) - 폰트 크기 및 굵기 업그레이드!
    lblTitle = new MarqueeLabel(leftWidget);
    lblTitle->setText("재생 중인 곡 없음");
    lblTitle->setAlignment(Qt::AlignCenter);
    // 제목은 제일 눈에 띄게 크고 굵게 (18px, bold)
    lblTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");

    lblArtist = new QLabel("아티스트 정보 없음", leftWidget);
    lblArtist->setAlignment(Qt::AlignCenter);
    // 아티스트는 중간 크기 (14px)
    lblArtist->setStyleSheet("color: #b3b3b3; font-size: 14px; font-weight: 500;");

    lblAlbum = new QLabel("앨범 정보 없음", leftWidget);
    lblAlbum->setAlignment(Qt::AlignCenter);
    // 앨범명은 살짝 작게 (13px)
    lblAlbum->setStyleSheet("color: #888888; font-size: 13px;");

    lblSpecs = new QLabel("Audio Spec: -", leftWidget);
    lblSpecs->setAlignment(Qt::AlignCenter);
    // 오디오 스펙도 기존 11px에서 12px로 살짝 키움
    lblSpecs->setStyleSheet("color: #666666; font-size: 12px;");

    leftLayout->addWidget(lblTitle);
    leftLayout->addWidget(lblArtist);
    leftLayout->addWidget(lblAlbum);
    leftLayout->addWidget(lblSpecs);

    // [핵심 포인트] 상단 곡정보와 하단 버튼들 사이를 밀어내는 강력한 공간(스프링)!
    leftLayout->addStretch();

    // --------------------------------------------------
    // 1-B. [하단 영역] 재생 슬라이더 & 컨트롤 버튼들 (바닥 고정)
    // --------------------------------------------------
    // 재생 위치 슬라이더
    sliderPosition = new QSlider(Qt::Horizontal, leftWidget);
    sliderPosition->setRange(0, 1000);
    sliderPosition->setValue(0);
    sliderPosition->installEventFilter(this);
    leftLayout->addWidget(sliderPosition);

    // 재생 제어 버튼 (셔플, 이전, 재생, 다음, 반복)
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    btnShuffle = new QPushButton("🔀", leftWidget);
    btnPrev = new QPushButton("⏮", leftWidget);
    btnPlayPause = new QPushButton("▶", leftWidget);
    btnNext = new QPushButton("⏭", leftWidget);
    btnRepeat = new QPushButton("🔁", leftWidget);

    btnPlayPause->setMinimumWidth(50);

    btnLayout->addWidget(btnShuffle);
    btnLayout->addWidget(btnPrev);
    btnLayout->addWidget(btnPlayPause);
    btnLayout->addWidget(btnNext);
    btnLayout->addWidget(btnRepeat);

    leftLayout->addLayout(btnLayout);

    // 볼륨 컨트롤
    auto* volLayout = new QHBoxLayout();
    auto* lblVolIcon = new QLabel("🔊", leftWidget);
    sliderVolume = new QSlider(Qt::Horizontal, leftWidget);
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(80);

    volLayout->addWidget(lblVolIcon);
    volLayout->addWidget(sliderVolume);
    leftLayout->addLayout(volLayout);

    // 파일 / 폴더 열기 버튼
    auto* openLayout = new QHBoxLayout();
    btnFileOpen = new QPushButton("파일 열기", leftWidget);
    btnFolderOpen = new QPushButton("폴더 열기", leftWidget);

    openLayout->addWidget(btnFileOpen);
    openLayout->addWidget(btnFolderOpen);
    leftLayout->addLayout(openLayout);

    // ==================================================
    // 2. [우측 패널] - 재생목록 테이블 영역
    // ==================================================
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(10, 20, 20, 20);

    playlistTable = new QTableWidget(rightWidget);
    playlistTable->setColumnCount(5);
    playlistTable->setHorizontalHeaderLabels({"제목", "아티스트", "앨범", "재생시간", "경로"});
    playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistTable->setAlternatingRowColors(true);

    // ==========================================
    // [수정된 부분] 비율 조정 확실하게 픽스!
    // ==========================================
    // 1. 충돌을 일으키던 '마지막 열 자동 늘림' 옵션 끄기
    playlistTable->horizontalHeader()->setStretchLastSection(false);

    // 2. 각 칸의 늘어나는 성질 지정
    playlistTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);     // 제목: 남는 공간 다 먹기
    playlistTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive); // 아티스트: 마우스로 크기 조절 가능
    playlistTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive); // 앨범: 마우스로 크기 조절 가능
    playlistTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);       // 재생시간: 크기 고정

    // 3. 1, 2, 3열의 기본 너비 세팅
    playlistTable->setColumnWidth(1, 150); // 아티스트 칸
    playlistTable->setColumnWidth(2, 200); // 앨범 칸
    playlistTable->setColumnWidth(3, 80);  // 재생시간 칸 (03:45 텍스트가 쏙 들어갈 크기)

    // 4. 경로(4열) 숨기기
    playlistTable->setColumnHidden(4, true);

    rightLayout->addWidget(playlistTable);

    // ==================================================
    // 3. [스플리터 구성 및 메인 레이아웃 추가]
    // ==================================================
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // ==================================================
    // 4. [시그널 - 슬롯 이벤트 연결]
    // ==================================================
    connect(btnPlayPause, &QPushButton::clicked, this, &MusicPlayerWindow::slotPlayPause);
    connect(btnPrev, &QPushButton::clicked, this, &MusicPlayerWindow::slotPrev);
    connect(btnNext, &QPushButton::clicked, this, &MusicPlayerWindow::slotNext);
    connect(btnFileOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFile);
    connect(btnFolderOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFolder);

    connect(sliderPosition, &QSlider::sliderReleased, this, &MusicPlayerWindow::slotSeek);
    connect(sliderVolume, &QSlider::valueChanged, this, &MusicPlayerWindow::slotVolumeChanged);

    connect(playlistTable, &QTableWidget::cellDoubleClicked, this, &MusicPlayerWindow::slotPlayTableItem);
    connect(playlistTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &MusicPlayerWindow::slotSortTable);

    connect(btnShuffle, &QPushButton::clicked, this, [this]() {
        isShuffle = !isShuffle;
        btnShuffle->setStyleSheet(isShuffle ? "background-color: #1DB954; color: white;" : "");
    });

    connect(btnRepeat, &QPushButton::clicked, this, [this]() {
        isRepeat = !isRepeat;
        btnRepeat->setStyleSheet(isRepeat ? "background-color: #1DB954; color: white;" : "");
    });

    // ==================================================
    // 5. [단축키 설정]
    // ==================================================
    auto* spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);

    // [추가] 스페이스바를 꾹 누르고 있어도 연속 입력이 안 되도록 차단!
    spaceShortcut->setAutoRepeat(false);

    connect(spaceShortcut, &QShortcut::activated, this, &MusicPlayerWindow::slotPlayPause);


    // inspired by KDE Breeze
    this->setStyleSheet(R"(
        QWidget {
            background-color: #1b1e20; /* Breeze Dark 메인 배경 */
            color: #fcfcfc;
        }
        QTableWidget {
            background-color: #232629; /* 리스트 배경 */
            alternate-background-color: #2a2e32; /* 줄바꿈 교차 배경 */
            border: none;
            gridline-color: #31363b;
            selection-background-color: #3daee9; /* Breeze 시그니처 하이라이트 블루! */
            selection-color: #ffffff;
        }
        QHeaderView::section {
            background-color: #31363b;
            color: #eff0f1;
            padding: 5px;
            border: none;
            font-weight: bold;
        }
        QPushButton {
            background-color: transparent;
            color: #ffffff;
            border-radius: 5px;
            padding: 5px 10px;
            font-size: 14pt;
        }
        QPushButton:hover {
            background-color: #31363b;
        }
        QSlider::groove:horizontal {
            border-radius: 2px;
            height: 4px;
            background: #50575e;
        }
        QSlider::handle:horizontal {
            background: #3daee9; /* 볼륨/재생 바 핸들도 Breeze 블루! */
            width: 14px;
            height: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
    )");
}

// 슬라이더의 빈 공간을 클릭했을 때 그 위치로 뿅! 하고 점프하는 고급 마법
bool MusicPlayerWindow::eventFilter(QObject *obj, QEvent *event) {
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

    // 추가: 스펙 텍스트를 한 번만 포맷팅해서 테이블 아이템의 비밀 주머니(+3)에 숨겨둠!
    QString channelStr = (meta.channels == 2) ? "Stereo" : (meta.channels == 1 ? "Mono" : QString::number(meta.channels) + " Ch");
    QString bitDepthStr = (meta.bitDepth > 0) ? QString("%1-bit | ").arg(meta.bitDepth) : "";
    QString specText = QString("🎵 %1%2 kHz | %3 kbps | %4")
                           .arg(bitDepthStr)
                           .arg(meta.sampleRate / 1000.0, 0, 'f', 1)
                           .arg(meta.bitrate)
                           .arg(channelStr);
    itemTitle->setData(Qt::UserRole + 3, specText);

    playlistTable->setItem(row, 0, itemTitle);
    playlistTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(meta.artist))); // NOLINT
    playlistTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(meta.album))); // NOLINT

    playlistTable->setSortingEnabled(wasSortingEnabled);
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

        lblSpecs->setText(item->data(Qt::UserRole + 3).toString());

        /*
        MusicMetadata meta = AudioEngine::getMetadata(path.toStdString());
        QString channelStr = (meta.channels == 2) ? "Stereo" : (meta.channels == 1 ? "Mono" : QString::number(meta.channels) + " Ch");
        QString bitDepthStr = (meta.bitDepth > 0) ? QString("%1-bit | ").arg(meta.bitDepth) : "";
        QString specText = QString("🎵 %1%2 kHz | %3 kbps | %4")
                               .arg(bitDepthStr)
                               .arg(meta.sampleRate / 1000.0, 0, 'f', 1)
                               .arg(meta.bitrate)
                               .arg(channelStr);
        lblSpecs->setText(specText); */

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

void MusicPlayerWindow::slotUpdateProgress() {
    double total = player.getTotalTime();
    double current = player.getCurrentTime();

    // 재생 중인 곡이 없거나 길이가 0이면 진행하지 않음
    if (total <= 0) return;

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
    savePlaylist();
    QWidget::closeEvent(event);
}

// 1. 앱 꺼질 때 모든 데이터를 JSON 객체로 예쁘게 포장해서 저장
void MusicPlayerWindow::savePlaylist() const {
    QJsonArray playlistArray;
    for (int row = 0; row < playlistTable->rowCount(); ++row) {
        QTableWidgetItem* item = playlistTable->item(row, 0);
        if (item) {
            QJsonObject songObj;
            songObj["path"] = item->data(Qt::UserRole).toString();
            songObj["track"] = item->data(Qt::UserRole + 1).toInt();
            songObj["disc"] = item->data(Qt::UserRole + 2).toInt();
            songObj["specs"] = item->data(Qt::UserRole + 3).toString();
            songObj["title"] = item->text();
            songObj["artist"] = playlistTable->item(row, 1)->text();
            songObj["album"] = playlistTable->item(row, 2)->text();
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

// 2. 앱 켤 때 오디오 엔진 안 거치고 다이렉트로 표에 꽂아버림! (부팅속도 극강)
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

        int row = playlistTable->rowCount();
        playlistTable->insertRow(row);

        auto* itemTitle = new QTableWidgetItem(obj["title"].toString()); // NOLINT
        itemTitle->setData(Qt::UserRole, path);
        itemTitle->setData(Qt::UserRole + 1, obj["track"].toInt());
        itemTitle->setData(Qt::UserRole + 2, obj["disc"].toInt());
        itemTitle->setData(Qt::UserRole + 3, obj["specs"].toString());

        playlistTable->setItem(row, 0, itemTitle);
        playlistTable->setItem(row, 1, new QTableWidgetItem(obj["artist"].toString())); // NOLINT
        playlistTable->setItem(row, 2, new QTableWidgetItem(obj["album"].toString())); // NOLINT
    }
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
        bool isPlaying; // 정렬 후에도 현재 재생 곡을 잃어버리지 않기 위해 기억!
    };

    std::vector<RowItems> rows;
    rows.reserve(playlistTable->rowCount());

    for (int r = 0; r < playlistTable->rowCount(); ++r) {
        rows.push_back({
            playlistTable->takeItem(r, 0), // takeItem: 테이블에서 뽑아옴 (삭제 안 됨)
            playlistTable->takeItem(r, 1),
            playlistTable->takeItem(r, 2),
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

// 창 크기가 바뀔 때마다 앨범 아트도 비율에 맞춰 깔끔하게 재계산
void MusicPlayerWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateAlbumArtDisplay();
}

MusicPlayerWindow::~MusicPlayerWindow() = default;