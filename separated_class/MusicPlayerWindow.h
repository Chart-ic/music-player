#ifndef MUSICPLAYERWINDOW_H
#define MUSICPLAYERWINDOW_H

#include <QPushButton>
#include <QTableWidget>
#include <QSlider>
#include <QTimer>
#include <QComboBox>
#include "AudioEngine.h"
#include "MarqueeLabel.h"

class QSystemTrayIcon;


class MusicPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr);
    ~MusicPlayerWindow() override;

    static QString customFontFamily;
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // 창 크기 변경 감지 이벤트 추가

private slots:
    void slotOpenFile();
    void slotOpenFolder();
    void slotPlayPause();
    void slotUpdateProgress();
    void slotSeek();
    void slotPlayTableItem(int row, int column);
    void slotVolumeChanged(int value);
    void slotNext();
    void slotPrev();
    void slotRemoveSelectedSong();
    void slotSortTable(int column); // 추가: 우리가 직접 제어할 정렬 함수!

private:
    void setupUI();
    void loadPlaylist() const;
    void savePlaylist() const;
    void loadSettings();
    void saveSettings() const;
    void addSongToTable(const QString& path, const MusicMetadata& meta) const;
    void updateAlbumArtDisplay() const; // 앨범 아트 갱신 함수 추가
    void updateAlbumArtGeometry() const;
    void updatePlaylistSummary() const;
    void setupTrayIcon();
    void restoreWindow();
    void resetNowPlaying();

    AudioEngine player;

    QLabel *lblAlbumArt{nullptr};
    MarqueeLabel *lblTitle{nullptr};
    QLabel *lblArtist{nullptr};
    QLabel *lblAlbum{nullptr};
    QLabel *lblSpecs{nullptr};
    QLabel *lblCurrentTime{nullptr};
    QLabel *lblTotalTime{nullptr};
    QLabel *lblPlaylistCount{nullptr};

    QTableWidget *playlistTable{nullptr};

    QSlider *sliderPosition{nullptr};
    QSlider *sliderVolume{nullptr};

    QPushButton *btnPlayPause{nullptr};
    QPushButton *btnPrev{nullptr};
    QPushButton *btnNext{nullptr};
    QPushButton *btnFileOpen{nullptr};
    QPushButton *btnFolderOpen{nullptr};

    QTimer *updateTimer{nullptr};

    QPushButton *btnShuffle{nullptr};
    QPushButton *btnRepeat{nullptr};
    QSystemTrayIcon *trayIcon{nullptr};
    qint64 lastPlayPauseTime = 0; // 추가: 마지막으로 재생/일시정지를 누른 시간
    QPixmap originalAlbumArt;     // 원본 앨범 아트 보관 변수

    bool isShuffle = false;
    bool isRepeat = false;
    bool isQuitting = false;
    bool trayHintShown = false;

    int currentRow = -1;
    bool isPlaying = false;
    void playSongFromTable(int row);

    // 추가: 현재 정렬 상태를 기억할 변수들
    Qt::SortOrder currentSortOrder = Qt::AscendingOrder;
    int currentSortColumn = -1;
};

#endif // MUSICPLAYERWINDOW_H
