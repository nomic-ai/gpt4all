import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import Qt5Compat.GraphicalEffects

Item {
    id: fileIcon
    property real iconSize: 24
    property string fileName: ""
    implicitWidth: iconSize
    implicitHeight: iconSize

    Image {
        id: fileImage
        anchors.fill: parent
        visible: false
        sourceSize.width: iconSize
        sourceSize.height: iconSize
        mipmap: true
        source: {
            if (fileIcon.fileName.toLowerCase().endsWith(".txt"))
                return "qrc:/gpt4all/icons/file-txt.svg"
            else if (fileIcon.fileName.toLowerCase().endsWith(".pdf"))
                return "qrc:/gpt4all/icons/file-pdf.svg"
            else if (fileIcon.fileName.toLowerCase().endsWith(".md"))
                return "qrc:/gpt4all/icons/file-md.svg"
            else if (fileIcon.fileName.toLowerCase().endsWith(".xlsx"))
                return "qrc:/gpt4all/icons/file-xls.svg"
            else if (fileIcon.fileName.toLowerCase().endsWith(".docx"))
                return "qrc:/gpt4all/icons/file-docx.svg"
            else
                return "qrc:/gpt4all/icons/file.svg"
        }
    }
    ColorOverlay {
        anchors.fill: fileImage
        source: fileImage
        color: theme.textColor
    }
}
