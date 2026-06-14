#ifndef MUSICPLAYERWINDOW_H
#define MUSICPLAYERWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QSlider>
#include <QTimer>
#include <QComboBox>
#include "AudioEngine.h"
#include "MarqueeLabel.h"


class MusicPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr);
    ~MusicPlayerWindow() override;

    static QString customFontFamily;
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    void closeEvent(QCloseEvent *event) override;

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
    void slotSortTable(int column); // 🌟 추가: 우리가 직접 제어할 정렬 함수!

private:
    void setupUI();
    void loadPlaylist() const;
    void savePlaylist() const;
    void addSongToTable(const QString& path, const MusicMetadata& meta) const;

    AudioEngine player;

    QLabel *lblAlbumArt;
    MarqueeLabel *lblTitle;
    QLabel *lblArtist;
    QLabel *lblAlbum;
    QLabel *lblSpecs;

    QTableWidget *playlistTable;

    QSlider *sliderPosition;
    QSlider *sliderVolume;

    QPushButton *btnPlayPause, *btnPrev, *btnNext;
    QPushButton *btnFileOpen, *btnFolderOpen;

    QTimer *updateTimer;

    QPushButton *btnShuffle;
    QPushButton *btnRepeat;

    bool isShuffle = false; // 🌟 셔플 상태 기억
    bool isRepeat = false;  // 🌟 한 곡 반복 상태 기억

    int currentRow = -1;
    bool isPlaying = false;
    void playSongFromTable(int row);

    // 🌟 추가: 현재 정렬 상태를 기억할 변수들
    Qt::SortOrder currentSortOrder = Qt::AscendingOrder;
    int currentSortColumn = -1;
};

#endif // MUSICPLAYERWINDOW_H