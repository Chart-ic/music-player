#ifndef MUSICPLAYERWINDOW_H
#define MUSICPLAYERWINDOW_H

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

    QLabel *lblAlbumArt{nullptr};
    MarqueeLabel *lblTitle{nullptr};
    QLabel *lblArtist{nullptr};
    QLabel *lblAlbum{nullptr};
    QLabel *lblSpecs{nullptr};

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
    qint64 lastPlayPauseTime = 0; // 🌟 추가: 마지막으로 재생/일시정지를 누른 시간

    bool isShuffle = false;
    bool isRepeat = false;

    int currentRow = -1;
    bool isPlaying = false;
    void playSongFromTable(int row);

    // 🌟 추가: 현재 정렬 상태를 기억할 변수들
    Qt::SortOrder currentSortOrder = Qt::AscendingOrder;
    int currentSortColumn = -1;
};

#endif // MUSICPLAYERWINDOW_H