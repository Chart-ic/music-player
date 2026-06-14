import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 400
    height: 800
    title: "PSMP Mobile"

    // 모던 다크 UI 감성 배경색
    color: "#1e1e1e"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // 1. 💿 앨범 아트 영역
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: width
            Layout.alignment: Qt.AlignHCenter
            color: "#333333"
            radius: 20

            Text {
                anchors.centerIn: parent
                text: "🎵\nNo Cover"
                color: "#888888"
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // 2. 📝 곡 정보 영역
        Column {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 5

            Text {
                width: parent.width
                text: "혁명전선 (Revolutionary Front)"
                color: "white"
                font.pixelSize: 28
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight // 글자가 길면 ... 처리
            }

            Text {
                width: parent.width
                text: "ツユ (TUYU)"
                color: "#aaaaaa"
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // 3. 🎚️ 재생 진행바 (Progress Slider)
        Slider {
            Layout.fillWidth: true
            from: 0
            to: 100
            value: 30 // 임시 값
        }

        // 4. ⏪ ▶️ ⏩ 재생 컨트롤 버튼 영역
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 30

            Button {
                text: "⏮"
                font.pixelSize: 30
                background: Rectangle { color: "transparent" }
                contentItem: Text { text: parent.text; color: "white"; font: parent.font }
            }

            Button {
                text: "▶️"
                font.pixelSize: 50
                background: Rectangle { color: "transparent" }
                contentItem: Text { text: parent.text; color: "white"; font: parent.font }

                // 🤝 나중에 여기에 C++ 브릿지 함수를 연결할 예정!
                // onClicked: { AndroidEngine.playPause() }
            }

            Button {
                text: "⏭"
                font.pixelSize: 30
                background: Rectangle { color: "transparent" }
                contentItem: Text { text: parent.text; color: "white"; font: parent.font }
            }
        }

        Item { Layout.fillHeight: true } // 하단 여백 밀어내기용 빈 공간
    }
}