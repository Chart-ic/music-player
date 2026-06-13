#ifndef MUSICPLAYERWINDOW_H
#define MUSICPLAYERWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QCloseEvent>
#include <QTimer>
#include <QListWidget>
#include <QComboBox>
#include "separated_class/AudioEngine.h" // 경로 일치 확인 필요 (프로젝트 루트 기준인지)

class MusicPlayerWindow : public QWidget {
    Q_OBJECT

public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr);
    ~MusicPlayerWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void slotPlay();
    void slotPause();
    void slotSeek(int value);
    void slotUpdateProgress() const;
    void slotOpenFile();
    void slotOpenFolder();
    void slotPlayListItem(const QListWidgetItem* item);
    void slotSortPlaylist(int index) const;

private:
    void savePlaylist() const;
    void loadPlaylist() const;

private:
    AudioEngine player;
    QTimer* updateTimer{nullptr};

    QLabel* lblTitle{nullptr};
    QLabel* lblArtist{nullptr};
    QLabel* lblAlbum{nullptr};
    QSlider* sliderPosition{nullptr};

    QPushButton* btnFileOpen{nullptr};
    QPushButton* btnFolderOpen{nullptr};
    QPushButton* btnPlay{nullptr};
    QPushButton* btnPause{nullptr};

    QListWidget* playlistWidget{nullptr};
    QComboBox* comboSort{nullptr};
};

#endif // MUSICPLAYERWINDOW_H