#include <QApplication>
#include <QDir>
#include <QSettings>
#include "mainwindow.h"
#include "theme/materialtheme.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("ROM Customization Toolkit");
    app.setOrganizationName("ROM Customization Toolkit");
    app.setApplicationVersion(APP_VERSION);

    // Same Material theme as the Android Toolbox project (Fusion + palette + app stylesheet)
    int themeNum = QSettings("ROM Customization Toolkit", "Settings").value("Theme", 0).toInt();
    if (themeNum < 0 || themeNum > 5) themeNum = 0;
    MaterialTheme::applyTheme(static_cast<MaterialTheme::ThemeType>(themeNum));

    MainWindow window;
    window.show();

    return app.exec();
}
