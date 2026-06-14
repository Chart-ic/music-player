#ifndef MARQUEELABEL_H
#define MARQUEELABEL_H

#include <QLabel>
#include <QString>
#include <QTimerEvent>
#include <QPaintEvent>

class MarqueeLabel : public QLabel {
    Q_OBJECT
public:
    explicit MarqueeLabel(QWidget* parent = nullptr);
    void setText(const QString& text);

protected:
    void timerEvent(QTimerEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QString fullText;
    int offset;
};

#endif // MARQUEELABEL_H