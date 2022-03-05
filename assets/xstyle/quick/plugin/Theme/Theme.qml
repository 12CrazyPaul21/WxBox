import QtQuick 2.8

QtObject {
    readonly property color gray: "#b2b1b1"
    readonly property color lightGray: "#dddddd"
    readonly property color light: "#ffffff"
    readonly property color blue: "#2d548b"    
    readonly property color dark: "#222222"
    readonly property color mainColorDarker: Qt.darker(mainColor, 1.5)

    readonly property int smallSize: 10
    readonly property int largeSize: 16

    property color mainColor: "#17a81a"
    property int baseSize: 10

    property font font
    font.bold: true
    font.underline: false
    font.pixelSize: 14
    font.family: "arial"

    property string backgroundImageUrl: ""

    function reset() {
        backgroundImageUrl = ""
    }
}
