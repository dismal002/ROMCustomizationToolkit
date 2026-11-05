#ifndef MATERIALTHEME_H
#define MATERIALTHEME_H

#include <QString>
#include <QColor>
#include <QApplication>

class MaterialTheme
{
public:
    enum ThemeType {
        LightBlue = 0,
        DarkTeal  = 1,
        DarkBlue  = 2,
        DarkPurple= 3,
        DarkGold  = 4,
        LightTeal = 5
    };

    struct Palette {
        bool isDark;
        QColor primary;
        QColor primaryLight;
        QColor primaryDark;
        QColor primaryPressed;
        QColor primaryText;
        QColor secondary;         // Window background
        QColor cardBg;            // Card / container background
        QColor inputBg;
        QColor inputBorder;
        QColor text;
        QColor textSecondary;
        QColor border;
        QColor sidebarBg;
        QColor sidebarHover;
        QColor sidebarSelected;
        QColor sidebarSelectedText;
        QColor sidebarText;
        QColor headerGradientStart;
        QColor headerGradientEnd;
        QColor headerBorder;
        QColor checkAccent;       // filled colour of checked checkboxes
    };

    static Palette getPalette(ThemeType theme);
    static QString getStyleSheet(ThemeType theme);
    static void applyTheme(ThemeType theme);
    static QString themeName(ThemeType theme);
};

#endif // MATERIALTHEME_H
