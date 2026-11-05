#ifndef ICONHELPER_H
#define ICONHELPER_H

#include <QIcon>
#include <QColor>
#include <QPixmap>

class IconHelper
{
public:
    enum IconType {
        Home,
        NavHome,
        Apps,
        Display,
        Sliders,
        Battery,
        Monitor,
        Script,
        Terminal,
        Settings,
        Refresh,
        Network,
        Back,
        Recents,
        Lock,
        VolumeUp,
        VolumeDown,
        Mute,
        Screenshot,
        Check,
        Restore,
        Info,
        Power
    };

    static QIcon createIcon(IconType type, const QColor &normalColor, const QColor &selectedColor = QColor(), int size = 32);
    static QPixmap createPixmap(IconType type, const QColor &color, int size = 32);

    static QIcon createIconFromSvg(const QString &svgPath, const QColor &normalColor, const QColor &selectedColor = QColor(), int size = 32);
    static QPixmap createPixmapFromSvg(const QString &svgPath, const QColor &color, int size = 32);
};

#endif // ICONHELPER_H
