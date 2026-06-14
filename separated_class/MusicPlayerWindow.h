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

class MusicPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr);
    ~MusicPlayerWindow() override;

    // font
    static QString customFontFamily;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void slotOpenFile();
    void slotOpenFolder();
    void slotPlay();
    void slotPause();
    void slotUpdateProgress() const; // 🌟 const 장착
    void slotSeek(int value);
    void slotPlayTableItem(int row, int column);
    void slotVolumeChanged(int value);
    void slotNext() const;           // 🌟 const 장착
    void slotPrev() const;           // 🌟 const 장착

private:
    void setupUI();
    void loadPlaylist() const;       // 🌟 const 장착
    void savePlaylist() const;       // 🌟 const 장착
    void addSongToTable(const QString& path, const MusicMetadata& meta) const;

    AudioEngine player;

    QLabel *lblAlbumArt;
    QLabel *lblTitle;
    QLabel *lblArtist;
    QLabel *lblAlbum;
    QLabel *lblSpecs;

    QTableWidget *playlistTable;

    QSlider *sliderPosition;
    QSlider *sliderVolume;

    QPushButton *btnPlay, *btnPause, *btnPrev, *btnNext;
    QPushButton *btnShuffle, *btnRepeat;
    QPushButton *btnFileOpen, *btnFolderOpen;

    QTimer *updateTimer;
};

#endif // MUSICPLAYERWINDOW_H