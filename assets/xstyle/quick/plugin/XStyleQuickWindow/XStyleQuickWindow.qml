import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.3
import QtGraphicalEffects 1.13
import Theme 1.0

import '../XStyleThemeDock.js' as ThemeDock

Rectangle {
    Component { 
        id: defaultTheme
        Theme{}
    }

    id: xcontainer 
    visible: true

    // an empty string represents the default theme
    property string currentThemeName: "";

    property Theme currentTheme: {
        return defaultTheme.createObject(xcontainer)
    }

    Connections {
        target: xwin
        onLanguageChanged: {
            retranslateUi();
        }
    }

    function retranslateUi() {
        //
        // retranslate all quick elements
        //

        // ......
    }

    function changeTheme(themeName) {

        if (themeName == currentThemeName) {
            return true;
        }

        // change main window theme
        if (!xwin.ChangeTheme(themeName)) {
            return false;
        }

        // update current theme name
        currentThemeName = themeName;

        // check whether default theme
        if (currentThemeName == "") {
            currentTheme = defaultTheme.createObject(xcontainer)
            return true;
        }

        // generate theme resource url
        var themeUrl = "qrc:/XStyleQuickThemes/" + themeName + "/" + themeName + ".qml";

        // instantiate theme object
        var themeObject = ThemeDock.selectTheme(xcontainer, themeUrl);
        if (!themeObject) {
            // if selectTheme failed, then use default theme
            currentTheme = defaultTheme.createObject(xcontainer)
            return false;
        }

        currentTheme = themeObject;
        return true;
    }

    function getThemeList() {
        return xwin.GetThemeList();
    }
}
