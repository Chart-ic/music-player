#include "MusicPlayerWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QFileInfo>
#include <QJsonDocument>
#include <QThread>     // background service
#include <QMetaObject> // 스레드 간 안전한 통신(보고)용
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDirIterator>
#include <QStyle>       // 슬라이더 클릭 위치 계산용
#include <QMouseEvent>  // 마우스 클릭 이벤트 감지용
#include <algorithm> // std::sort 사용을 위해 추가
#include <QRandomGenerator> // 셔플 기능을 위한 랜덤 엔진
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
    auto* mainLayout = new QHBoxLayout(this);

    // --- [좌측 섹션] ---
    auto* leftLayout = new QVBoxLayout(); // NOLINT
    leftLayout->setContentsMargins(20, 20, 20, 20);

    lblAlbumArt = new QLabel(this);
    lblAlbumArt->setFixedSize(300, 300);
    lblAlbumArt->setStyleSheet("background-color: #333; border-radius: 15px; color: white;");
    lblAlbumArt->setAlignment(Qt::AlignCenter);
    lblAlbumArt->setText("Album Art");

    lblTitle = new MarqueeLabel(this);
    lblTitle->setText("Song");

    // 🌟 추가: 전광판 글자가 길어도 레이아웃을 밀어내지 않고 마스킹되게 사이즈 정책 강제!
    lblTitle->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    lblTitle->setMinimumWidth(300); // 앨범 아트(300px)와 가로 길이 맞춤

    QFont titleFont(customFontFamily);
    titleFont.setPointSize(18);
    titleFont.setWeight(QFont::Bold);
    lblTitle->setFont(titleFont);

    lblArtist = new QLabel("Artist", this);
    lblAlbum = new QLabel("Album", this);

    lblSpecs = new QLabel("Specs", this);
    lblSpecs->setStyleSheet("font-size: 11pt; color: #aaaaaa;");

    // 재생 버튼 통합! + 컨트롤 레이아웃 부분 수정 (셔플과 반복 버튼 추가!)
    auto* ctrlLayout = new QHBoxLayout(); // NOLINT
    btnShuffle = new QPushButton("🔀", this);
    btnPrev = new QPushButton("⏮", this);
    btnPlayPause = new QPushButton("▶", this);
    btnNext = new QPushButton("⏭", this);
    btnRepeat = new QPushButton("🔁", this);

    btnShuffle->setCheckable(true); // 토글(On/Off) 가능하게 설정
    btnRepeat->setCheckable(true);

    ctrlLayout->addWidget(btnShuffle);
    ctrlLayout->addWidget(btnPrev);
    ctrlLayout->addWidget(btnPlayPause);
    ctrlLayout->addWidget(btnNext);
    ctrlLayout->addWidget(btnRepeat);

    sliderPosition = new QSlider(Qt::Horizontal, this);
    sliderPosition->installEventFilter(this); // 클릭으로 위치 이동하게 감지기 부착!

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
    leftLayout->addWidget(lblSpecs);
    leftLayout->addWidget(sliderPosition);
    leftLayout->addLayout(ctrlLayout);
    leftLayout->addLayout(volLayout);
    leftLayout->addStretch();

    // --- [우측 섹션] ---
    auto* rightLayout = new QVBoxLayout(); // NOLINT

    // 우측 버튼과 표의 폰트 크기를 작게(10pt) 줄이기!
    QFont smallFont(customFontFamily);
    smallFont.setPointSize(10);

    auto* topMenuLayout = new QHBoxLayout(); // NOLINT
    btnFileOpen = new QPushButton("파일 추가", this);
    btnFolderOpen = new QPushButton("폴더 추가", this);

    btnFileOpen->setFont(smallFont);   // 폰트 작게 적용
    btnFolderOpen->setFont(smallFont); // 폰트 작게 적용

    topMenuLayout->addWidget(btnFileOpen);
    topMenuLayout->addWidget(btnFolderOpen);
    topMenuLayout->addStretch();

    playlistTable = new QTableWidget(0, 3, this);
    playlistTable->setFont(smallFont); // 재생 목록 폰트도 작게 적용
    playlistTable->setHorizontalHeaderLabels({"제목", "아티스트", "앨범"});
    playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistTable->setAlternatingRowColors(true);
    playlistTable->setWordWrap(false); // 추가: 글자가 길어도 칸이 안 늘어나고 '...'으로 예쁘게 잘림!
    playlistTable->horizontalHeader()->setSortIndicatorShown(true);
    playlistTable->verticalHeader()->setVisible(false);

    playlistTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    // 1번(아티스트), 2번(앨범) 열은 사용자가 마우스로 크기 조절 가능하게 둠
    playlistTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    playlistTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);

    rightLayout->addLayout(topMenuLayout);
    rightLayout->addWidget(playlistTable);

    // 🌟 (이렇게 변경!)
    mainLayout->addLayout(leftLayout, 0);  // 좌측은 앨범아트(300px) 크기에 맞춰서 고정!
    mainLayout->addLayout(rightLayout, 1); // 창을 늘리면 우측(테이블)이 남는 공간을 다 빨아들임!

    // 이벤트 연결 (재생 슬라이더는 드래그 중(Moved)이 아니라 놨을 때(Released)만 반영되게 수정!)
    connect(btnPlayPause, &QPushButton::clicked, this, &MusicPlayerWindow::slotPlayPause);
    connect(btnFileOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFile);
    connect(btnFolderOpen, &QPushButton::clicked, this, &MusicPlayerWindow::slotOpenFolder);
    connect(playlistTable, &QTableWidget::cellDoubleClicked, this, &MusicPlayerWindow::slotPlayTableItem);
    connect(sliderPosition, &QSlider::sliderReleased, this, &MusicPlayerWindow::slotSeek);
    connect(sliderVolume, &QSlider::valueChanged, this, &MusicPlayerWindow::slotVolumeChanged);
    connect(btnNext, &QPushButton::clicked, this, &MusicPlayerWindow::slotNext);
    connect(btnPrev, &QPushButton::clicked, this, &MusicPlayerWindow::slotPrev);
    connect(playlistTable->horizontalHeader(), &QHeaderView::sectionClicked, this, &MusicPlayerWindow::slotSortTable);

    // 2. 이벤트 연결 부분에 셔플/반복 클릭 이벤트 추가
    // 셔플/반복 클릭 시 활성화되는 색상을 초록색에서 #3daee9(Breeze 블루)로 교체!
    connect(btnShuffle, &QPushButton::clicked, this, [this](bool checked) {
        isShuffle = checked;
        btnShuffle->setStyleSheet(checked ? "color: #3daee9; font-weight: bold;" : "color: #ffffff;");
    });
    connect(btnRepeat, &QPushButton::clicked, this, [this](bool checked) {
        isRepeat = checked;
        btnRepeat->setStyleSheet(checked ? "color: #3daee9; font-weight: bold;" : "color: #ffffff;");
    });

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
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
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

    // 🌟 추가: 스펙 텍스트를 한 번만 포맷팅해서 테이블 아이템의 비밀 주머니(+3)에 숨겨둠!
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

        QByteArray artData = AudioEngine::getAlbumArt(path.toStdString());
        if (!artData.isEmpty()) {
            QPixmap pixmap;
            pixmap.loadFromData(artData);
            lblAlbumArt->setPixmap(pixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            lblAlbumArt->clear();
            lblAlbumArt->setText("No Cover Art");
        }

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

    if (!sliderPosition->isSliderDown()) {
        sliderPosition->setValue(static_cast<int>(current));
    }

    // 노래가 끝났을 때의 행동 결정
    if (currentRow >= 0 && total > 0 && current >= total - 0.1) {
        if (isRepeat) {
            // 반복이 켜져 있으면 위치를 0으로 돌리고 다시 재생!
            player.setPosition(0);
            player.play();
        } else {
            // 꺼져 있으면 자연스럽게 다음 곡으로
            slotNext();
        }
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
    if (dirPath.isEmpty()) return;

    // 🌟 버튼 텍스트를 바꿔서 로딩 중임을 알림 (UI 응답성)
    btnFolderOpen->setText("로딩 중...");
    btnFolderOpen->setEnabled(false);

    // 🌟 일꾼 스레드(알바생) 하나 고용해서 백그라운드로 보냄!
    QThread::create([this, dirPath]() {
        QDirIterator it(dirPath, {"*.flac", "*.mp3", "*.opus", "*.wav", "*.ogg"}, QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString path = it.next();
            // 디스크를 긁는 무거운 작업은 백그라운드에서 진행
            MusicMetadata meta = AudioEngine::getMetadata(path.toStdString());

            // 🌟 분석이 끝나면 메인 스레드(사장)한테 "표에 추가해주세요!" 라고 안전하게 결재 올림
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
    player.setPosition(static_cast<double>(sliderPosition->value()));
}

void MusicPlayerWindow::closeEvent(QCloseEvent *event) {
    savePlaylist();
    QWidget::closeEvent(event);
}

// 🌟 1. 앱 꺼질 때 모든 데이터를 JSON 객체로 예쁘게 포장해서 저장
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

// 🌟 2. 앱 켤 때 오디오 엔진 안 거치고 다이렉트로 표에 꽂아버림! (부팅속도 극강)
void MusicPlayerWindow::loadPlaylist() const {
    QFile file("playlist.json");
    if (!file.open(QIODevice::ReadOnly)) return;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return;

    for (const QJsonValue& value : doc.array()) {
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

    // 3. 대망의 다중 정렬 알고리즘 🌟
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
    playlistTable->setRowCount(rows.size());
    currentRow = -1;

    for (int r = 0; r < rows.size(); ++r) {
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

MusicPlayerWindow::~MusicPlayerWindow() {}