.import QtQuick 2.0 as QtQuick

function selectTheme(item, themeRcPath) {
    var component = Qt.createComponent(themeRcPath);

    if (!component || component.status != QtQuick.Component.Ready) {
        console.log('[' + themeRcPath + '] Instantiation of theme object failed : ' + component.errorString());
        return null;
    }

    var sprite = component.createObject(item)
    if (!sprite) {
        console.log("Instantiation of theme object failed");
    }

    return sprite;
}