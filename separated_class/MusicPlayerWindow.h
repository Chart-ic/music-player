#ifndef MUSICPLAYERWINDOW_H
#define MUSICPLAYERWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QTimer>
#include <QListWidget> // 🌟 1. 리스트 위젯 헤더 추가!
#include "separated_class/AudioEngine.h"

class MusicPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr);
    ~MusicPlayerWindow() override;

private slots:
    void slotPlay();
    void slotPause();
    void slotSeek(int value);
    void slotUpdateProgress();
    void slotOpenFile();
    void slotPlayListItem(const QListWidgetItem* item); // 🌟 2. 리스트 더블클릭 시 실행될 슬롯!

private:
    AudioEngine player;
    QTimer* updateTimer;

    QLabel* lblTitle;
    QLabel* lblArtist;
    QLabel* lblAlbum;
    QSlider* sliderPosition;

    QPushButton* btnFileOpen;
    QPushButton* btnPlay;
    QPushButton* btnPause;

    QListWidget* playlistWidget; // 🌟 3. 리스트 UI 변수 선언!
};

#endif // MUSICPLAYERWINDOW_H