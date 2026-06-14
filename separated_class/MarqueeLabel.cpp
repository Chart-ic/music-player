#include "MarqueeLabel.h"
#include <QPainter>
#include <QFontMetrics>

MarqueeLabel::MarqueeLabel(QWidget* parent) : QLabel(parent), offset(0) {
    startTimer(30); // 30ms마다 화면 갱신
}

void MarqueeLabel::setText(const QString& text) {
    fullText = text;
    offset = 0; // 새 곡 재생 시 위치 초기화
    QLabel::setText(text);
}

void MarqueeLabel::timerEvent(QTimerEvent *event) {
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(fullText);
    
    if (textWidth > width()) {
        offset -= 1; // 왼쪽으로 1픽셀씩 이동
        if (offset < -textWidth) {
            offset = width(); // 화면 밖으로 나가면 오른쪽에서 다시 등장
        }
        update();
    } else {
        offset = 0;
    }
}

void MarqueeLabel::paintEvent(QPaintEvent *event) {
    QFontMetrics fm(font());
    
    if (fm.horizontalAdvance(fullText) > width()) {
        QPainter p(this);

        // 🌟 추가: 다크모드 테마의 글자색(하얀색/밝은회색)을 가져와서 그리기 펜에 장착!
        p.setPen(palette().color(QPalette::WindowText));

        p.setClipRect(rect()); // 라벨 크기 밖 텍스트 마스킹

        QRect r = rect();
        r.setLeft(offset);
        r.setWidth(fm.horizontalAdvance(fullText) + 50);
        p.drawText(r, Qt::AlignVCenter | Qt::AlignLeft, fullText);
    } else {
        QLabel::paintEvent(event);
    }
}