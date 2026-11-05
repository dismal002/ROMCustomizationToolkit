#include "materialtheme.h"

#include <QStyleFactory>
#include <QPalette>
#include <QApplication>

MaterialTheme::Palette MaterialTheme::getPalette(ThemeType theme)
{
    Palette p;
    switch (theme) {
    case DarkTeal:
        p.isDark              = true;
        p.primary             = QColor("#009688"); // Teal
        p.primaryLight        = QColor("#4DB6AC");
        p.primaryDark         = QColor("#00796B");
        p.primaryPressed      = QColor("#004D40");
        p.primaryText         = QColor("#FFFFFF");
        p.secondary           = QColor("#1E272C"); // Dark background
        p.cardBg              = QColor("#263238");
        p.inputBg             = QColor("#1E272C");
        p.inputBorder         = QColor("#37474F");
        p.text                = QColor("#ECEFF1");
        p.textSecondary       = QColor("#90A4AE");
        p.border              = QColor("#37474F");
        p.sidebarBg           = QColor("#192125");
        p.sidebarHover        = QColor("#222E34");
        p.sidebarSelected     = QColor("#1A3A35");
        p.sidebarSelectedText = QColor("#4DB6AC");
        p.sidebarText         = QColor("#B0BEC5");
        p.headerGradientStart = QColor("#00796B");
        p.headerGradientEnd   = QColor("#004D40");
        p.headerBorder        = QColor("#004D40");
        break;

    case DarkBlue:
        p.isDark              = true;
        p.primary             = QColor("#2196F3"); // Material Blue
        p.primaryLight        = QColor("#64B5F6");
        p.primaryDark         = QColor("#1976D2");
        p.primaryPressed      = QColor("#0D47A1");
        p.primaryText         = QColor("#FFFFFF");
        p.secondary           = QColor("#171E28");
        p.cardBg              = QColor("#212B36");
        p.inputBg             = QColor("#171E28");
        p.inputBorder         = QColor("#334155");
        p.text                = QColor("#F1F5F9");
        p.textSecondary       = QColor("#94A3B8");
        p.border              = QColor("#334155");
        p.sidebarBg           = QColor("#121820");
        p.sidebarHover        = QColor("#1E293B");
        p.sidebarSelected     = QColor("#1E3553");
        p.sidebarSelectedText = QColor("#64B5F6");
        p.sidebarText         = QColor("#94A3B8");
        p.headerGradientStart = QColor("#1976D2");
        p.headerGradientEnd   = QColor("#0D47A1");
        p.headerBorder        = QColor("#0D47A1");
        break;

    case DarkPurple:
        p.isDark              = true;
        p.primary             = QColor("#7E57C2"); // Deep Purple
        p.primaryLight        = QColor("#B39DDB");
        p.primaryDark         = QColor("#5E35B1");
        p.primaryPressed      = QColor("#4527A0");
        p.primaryText         = QColor("#FFFFFF");
        p.secondary           = QColor("#191624");
        p.cardBg              = QColor("#252136");
        p.inputBg             = QColor("#191624");
        p.inputBorder         = QColor("#3D3656");
        p.text                = QColor("#EDE7F6");
        p.textSecondary       = QColor("#B39DDB");
        p.border              = QColor("#3D3656");
        p.sidebarBg           = QColor("#14111E");
        p.sidebarHover        = QColor("#2D2842");
        p.sidebarSelected     = QColor("#31264E");
        p.sidebarSelectedText = QColor("#B39DDB");
        p.sidebarText         = QColor("#B39DDB");
        p.headerGradientStart = QColor("#5E35B1");
        p.headerGradientEnd   = QColor("#4527A0");
        p.headerBorder        = QColor("#4527A0");
        break;

    case DarkGold:
        p.isDark              = true;
        p.primary             = QColor("#FFA000"); // Amber
        p.primaryLight        = QColor("#FFD54F");
        p.primaryDark         = QColor("#FF8F00");
        p.primaryPressed      = QColor("#FF6F00");
        p.primaryText         = QColor("#000000");
        p.secondary           = QColor("#1C1A17");
        p.cardBg              = QColor("#282520");
        p.inputBg             = QColor("#1C1A17");
        p.inputBorder         = QColor("#423D33");
        p.text                = QColor("#FFF8E1");
        p.textSecondary       = QColor("#FFE082");
        p.border              = QColor("#423D33");
        p.sidebarBg           = QColor("#151310");
        p.sidebarHover        = QColor("#332E27");
        p.sidebarSelected     = QColor("#3E3218");
        p.sidebarSelectedText = QColor("#FFD54F");
        p.sidebarText         = QColor("#FFE082");
        p.headerGradientStart = QColor("#C47D00");
        p.headerGradientEnd   = QColor("#8F5A00");
        p.headerBorder        = QColor("#8F5A00");
        break;

    case LightTeal:
        p.isDark              = false;
        p.primary             = QColor("#00897B");
        p.primaryLight        = QColor("#80CBC4");
        p.primaryDark         = QColor("#00695C");
        p.primaryPressed      = QColor("#004D40");
        p.primaryText         = QColor("#FFFFFF");
        p.secondary           = QColor("#F4FBF9");
        p.cardBg              = QColor("#FFFFFF");
        p.inputBg             = QColor("#FFFFFF");
        p.inputBorder         = QColor("#B2DFDB");
        p.text                = QColor("#1F2937");
        p.textSecondary       = QColor("#6B7280");
        p.border              = QColor("#E0F2F1");
        p.sidebarBg           = QColor("#EAF6F4");
        p.sidebarHover        = QColor("#D9ECE9");
        p.sidebarSelected     = QColor("#D1EFEA");
        p.sidebarSelectedText = QColor("#00897B");
        p.sidebarText         = QColor("#374151");
        p.headerGradientStart = QColor("#00897B");
        p.headerGradientEnd   = QColor("#00695C");
        p.headerBorder        = QColor("#004D40");
        break;

    case LightBlue:
    default:
        // Default Material Light Blue - Matches Image 1!
        p.isDark              = false;
        p.primary             = QColor("#3F51B5"); // Indigo / Material Blue #3949AB
        p.primaryLight        = QColor("#C5CAE9");
        p.primaryDark         = QColor("#303F9F");
        p.primaryPressed      = QColor("#1A237E");
        p.primaryText         = QColor("#FFFFFF");
        p.secondary           = QColor("#F4F6F9"); // Clean app background
        p.cardBg              = QColor("#FFFFFF"); // Pure white cards
        p.inputBg             = QColor("#FFFFFF");
        p.inputBorder         = QColor("#D1D5DB");
        p.text                = QColor("#1E293B"); // Slate 800
        p.textSecondary       = QColor("#64748B"); // Slate 500
        p.border              = QColor("#E2E8F0"); // Slate 200
        p.sidebarBg           = QColor("#F8FAFC"); // Slate 50
        p.sidebarHover        = QColor("#EEF2F6");
        p.sidebarSelected     = QColor("#E0E7FF"); // Indigo 100 pill
        p.sidebarSelectedText = QColor("#3F51B5");
        p.sidebarText         = QColor("#475569"); // Slate 600
        p.headerGradientStart = QColor("#3F51B5"); // Vibrant Material Indigo Header
        p.headerGradientEnd   = QColor("#303F9F");
        p.headerBorder        = QColor("#283593");
        p.checkAccent         = QColor("#6200EE"); // Material Purple 500
        break;
    }
    if (!p.checkAccent.isValid()) p.checkAccent = p.primary;
    return p;
}

QString MaterialTheme::themeName(ThemeType theme)
{
    switch (theme) {
    case DarkTeal:   return QStringLiteral("Dark Teal");
    case DarkBlue:   return QStringLiteral("Dark Blue");
    case DarkPurple: return QStringLiteral("Dark Purple");
    case DarkGold:   return QStringLiteral("Dark Gold");
    case LightTeal:  return QStringLiteral("Light Teal");
    case LightBlue:
    default:         return QStringLiteral("Light Blue (Default)");
    }
}

QString MaterialTheme::getStyleSheet(ThemeType theme)
{
    Palette p = getPalette(theme);

    QString pri = p.primary.name();
    QString priLight = p.primaryLight.name();
    QString priDark = p.primaryDark.name();
    QString priPressed = p.primaryPressed.name();
    QString priText = p.primaryText.name();
    QString sec = p.secondary.name();
    QString card = p.cardBg.name();
    QString inpBg = p.inputBg.name();
    QString inpBorder = p.inputBorder.name();
    QString txt = p.text.name();
    QString txtSec = p.textSecondary.name();
    QString bdr = p.border.name();
    QString sideBg = p.sidebarBg.name();
    QString sideHov = p.sidebarHover.name();
    QString sideSel = p.sidebarSelected.name();
    QString sideSelTxt = p.sidebarSelectedText.name();
    QString sideTxt = p.sidebarText.name();
    QString headStart = p.headerGradientStart.name();
    QString headEnd = p.headerGradientEnd.name();
    QString headBdr = p.headerBorder.name();

    QString scrollThumb = p.isDark ? "#4B5563" : "#CBD5E1";
    QString scrollThumbHov = p.isDark ? "#6B7280" : "#94A3B8";
    QString disabledBg = p.isDark ? "#2D3748" : "#E2E8F0";
    QString disabledText = p.isDark ? "#718096" : "#94A3B8";

    QString css = QString(R"(
        /* ── Base Window & Font ─────────────────────────────── */
        QWidget {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", "Roboto", "Helvetica Neue", "PingFang SC", "Microsoft YaHei", sans-serif;
            font-size: 13px;
            color: %1;
        }

        QMainWindow {
            background-color: %2;
        }

        QDialog {
            background-color: %2;
        }

        /* ── Top Header Bar ─────────────────────────────────── */
        QWidget#headerBar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %3, stop:1 %4);
            border-bottom: 1px solid %5;
            min-height: 52px;
            max-height: 52px;
        }

        QWidget#headerBar QLabel {
            color: #FFFFFF;
            background: transparent;
        }

        QWidget#headerBar QLabel#headerTitle {
            color: #FFFFFF;
            font-size: 15px;
            font-weight: bold;
        }

        QWidget#headerBar QLabel#headerSubtitle {
            color: rgba(255, 255, 255, 0.85);
            font-size: 12px;
        }

        QWidget#headerBar QComboBox {
            background-color: rgba(255, 255, 255, 0.95);
            color: #1E293B;
            border: 1px solid rgba(255, 255, 255, 0.3);
            border-radius: 6px;
            padding: 4px 10px;
            min-height: 24px;
            font-weight: 500;
        }

        QWidget#headerBar QPushButton {
            background-color: rgba(255, 255, 255, 0.18);
            color: #FFFFFF;
            border: 1px solid rgba(255, 255, 255, 0.35);
            border-radius: 6px;
            padding: 5px 14px;
            min-height: 24px;
            font-weight: 500;
        }

        QWidget#headerBar QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.30);
            border-color: rgba(255, 255, 255, 0.6);
        }

        QWidget#headerBar QPushButton:pressed {
            background-color: rgba(255, 255, 255, 0.45);
        }

        /* ── Left Sidebar Navigation ────────────────────────── */
        QWidget#sidebarContainer {
            background-color: %6;
            border-right: 1px solid %7;
        }

        QListWidget#sidebarList {
            background-color: transparent;
            border: none;
            padding: 6px 4px;
            outline: none;
        }

        QListWidget#sidebarList::item {
            height: 44px;
            border-radius: 8px;
            padding: 0px 12px;
            margin: 3px 6px;
            color: %8;
            font-size: 13px;
            font-weight: 500;
        }

        QListWidget#sidebarList::item:hover {
            background-color: %9;
            color: %1;
        }

        QListWidget#sidebarList::item:selected {
            background-color: %10;
            color: %24;
            font-weight: bold;
        }

        /* ── Material Cards (QGroupBox) ─────────────────────── */
        QGroupBox {
            background-color: %12;
            border: 1px solid %7;
            border-radius: 8px;
            margin-top: 20px;
            padding: 16px 12px 12px 12px;
            font-weight: 600;
            color: %1;
        }

        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 12px;
            padding: 2px 8px;
            background-color: %12;
            border-radius: 4px;
            color: %11;
            font-size: 13px;
            font-weight: bold;
        }

        /* ── Buttons (QPushButton) ──────────────────────────── */
        QPushButton {
            background-color: %11;
            color: %13;
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 500;
            font-size: 13px;
            min-height: 26px;
        }

        QPushButton:hover {
            background-color: %14;
        }

        QPushButton:pressed {
            background-color: %15;
        }

        QPushButton:disabled {
            background-color: %16;
            color: %17;
        }

        QPushButton[class="secondary"],
        QPushButton[flat="true"] {
            background-color: transparent;
            color: %11;
            border: 1px solid %11;
        }

        QPushButton[class="secondary"]:hover,
        QPushButton[flat="true"]:hover {
            background-color: %18;
        }

        /* ── Input Fields (QLineEdit, QSpinBox) ─────────────── */
        QLineEdit, QSpinBox, QDoubleSpinBox {
            background-color: %19;
            color: %1;
            border: 1px solid %20;
            border-radius: 6px;
            padding: 5px 10px;
            min-height: 22px;
            selection-background-color: %11;
            selection-color: %13;
        }

        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 2px solid %11;
            padding: 4px 9px;
        }

        QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {
            background-color: %16;
            color: %17;
            border-color: %7;
        }

        QSpinBox::up-button, QSpinBox::down-button,
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
            subcontrol-origin: border;
            width: 18px;
            border: none;
            background: transparent;
        }

        QSpinBox::up-button:hover, QSpinBox::down-button:hover,
        QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
            background-color: %9;
            border-radius: 3px;
        }

        /* ── Combo Boxes (QComboBox) ────────────────────────── */
        QComboBox {
            background-color: %19;
            color: %1;
            border: 1px solid %20;
            border-radius: 6px;
            padding: 5px 12px;
            min-height: 22px;
        }

        QComboBox:focus {
            border: 2px solid %11;
            padding: 4px 11px;
        }

        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 22px;
            border-left: none;
        }

        QComboBox QAbstractItemView {
            background-color: %12;
            color: %1;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 4px;
            selection-background-color: %10;
            selection-color: %11;
            outline: none;
        }

        /* ── Lists (QListWidget) ────────────────────────────── */
        QListWidget {
            background-color: %19;
            color: %1;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 4px;
            outline: none;
        }

        QListWidget::item {
            height: 28px;
            border-radius: 4px;
            padding: 2px 8px;
            margin: 1px 0px;
        }

        QListWidget::item:hover {
            background-color: %9;
        }

        QListWidget::item:selected {
            background-color: %10;
            color: %11;
            font-weight: 600;
        }

        /* Apps tab icon grid: the generic 28px item height clips the label below the icon */
        QListWidget#appsViewer::item {
            height: 100px;
            padding: 4px;
        }

        /* ── Scrollbars (QScrollBar) ────────────────────────── */
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 0;
        }

        QScrollBar::handle:vertical {
            background: %21;
            border-radius: 4px;
            min-height: 30px;
        }

        QScrollBar::handle:vertical:hover {
            background: %22;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
            height: 0;
        }

        QScrollBar:horizontal {
            background: transparent;
            height: 8px;
            margin: 0;
        }

        QScrollBar::handle:horizontal {
            background: %21;
            border-radius: 4px;
            min-width: 30px;
        }

        QScrollBar::handle:horizontal:hover {
            background: %22;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
            width: 0;
        }

        /* ── Text Areas (QTextEdit, QPlainTextEdit) ─────────── */
        QTextEdit, QPlainTextEdit {
            background-color: %19;
            color: %1;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 8px;
            font-family: "Cascadia Code", "Fira Code", "Consolas", "Courier New", monospace;
        }

        /* ── Sliders (QSlider) ──────────────────────────────── */
        QSlider::groove:horizontal {
            height: 6px;
            background: %7;
            border-radius: 3px;
        }

        QSlider::sub-page:horizontal {
            background: %11;
            border-radius: 3px;
        }

        QSlider::handle:horizontal {
            background: %11;
            width: 18px;
            height: 18px;
            margin: -6px 0;
            border-radius: 9px;
            border: 2px solid #FFFFFF;
        }

        QSlider::handle:horizontal:hover {
            width: 20px;
            height: 20px;
            margin: -7px 0;
            border-radius: 10px;
        }

        /* ── CheckBoxes & RadioButtons ──────────────────────── */
        QCheckBox, QRadioButton {
            spacing: 8px;
            color: %1;
        }

        QRadioButton::indicator {
            width: 18px;
            height: 18px;
        }

        /* ── Status Bar ─────────────────────────────────────── */
        QStatusBar {
            background-color: %12;
            color: %23;
            border-top: 1px solid %7;
            padding: 4px 12px;
            font-size: 12px;
        }

        /* ── Splitter ───────────────────────────────────────── */
        QSplitter::handle {
            background-color: %7;
        }
    )")
    .arg(txt)           // %1
    .arg(sec)           // %2
    .arg(headStart)     // %3
    .arg(headEnd)       // %4
    .arg(headBdr)       // %5
    .arg(sideBg)        // %6
    .arg(bdr)           // %7
    .arg(sideTxt)       // %8
    .arg(sideHov)       // %9
    .arg(sideSel)       // %10
    .arg(pri)           // %11
    .arg(card)          // %12
    .arg(priText)       // %13
    .arg(priDark)       // %14
    .arg(priPressed)    // %15
    .arg(disabledBg)    // %16
    .arg(disabledText)  // %17
    .arg(priLight)      // %18
    .arg(inpBg)         // %19
    .arg(inpBorder)     // %20
    .arg(scrollThumb)   // %21
    .arg(scrollThumbHov)// %22
    .arg(txtSec)        // %23
    .arg(sideSelTxt);   // %24

    // ── Material checkbox: filled rounded square with white check when checked ──
    css += QString(R"(
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border: 2px solid %1;
            border-radius: 4px;
            background-color: %2;
        }
        QCheckBox::indicator:hover {
            border-color: %3;
        }
        QCheckBox::indicator:checked, QCheckBox::indicator:indeterminate {
            background-color: %3;
            border-color: %3;
            image: url(:/icons/check_white.svg);
        }
        QCheckBox::indicator:disabled {
            background-color: %4;
            border-color: %5;
        }
        QCheckBox::indicator:checked:disabled {
            background-color: %5;
            border-color: %5;
            image: url(:/icons/check_white.svg);
        }
    )").arg(p.isDark ? "#94A3B8" : "#9CA3AF")   // %1 unchecked border
       .arg(p.inputBg.name())                   // %2 unchecked fill
       .arg(p.checkAccent.name())               // %3 accent
       .arg(disabledBg)                         // %4 disabled fill
       .arg(disabledText);                      // %5 disabled border / checked-disabled fill

    return css;
}

void MaterialTheme::applyTheme(ThemeType theme)
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    Palette pal = getPalette(theme);

    QPalette qpal;
    qpal.setColor(QPalette::Window, pal.secondary);
    qpal.setColor(QPalette::WindowText, pal.text);
    qpal.setColor(QPalette::Base, pal.inputBg);
    qpal.setColor(QPalette::AlternateBase, pal.cardBg);
    qpal.setColor(QPalette::ToolTipBase, pal.cardBg);
    qpal.setColor(QPalette::ToolTipText, pal.text);
    qpal.setColor(QPalette::Text, pal.text);
    qpal.setColor(QPalette::Button, pal.primary);
    qpal.setColor(QPalette::ButtonText, pal.primaryText);
    qpal.setColor(QPalette::BrightText, Qt::red);
    qpal.setColor(QPalette::Link, pal.primary);
    qpal.setColor(QPalette::Highlight, pal.primary);
    qpal.setColor(QPalette::HighlightedText, pal.primaryText);

    if (pal.isDark) {
        qpal.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
        qpal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));
        qpal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
        qpal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(60, 60, 60));
    } else {
        qpal.setColor(QPalette::Disabled, QPalette::Text, QColor(160, 160, 160));
        qpal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
        qpal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
        qpal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(210, 210, 210));
    }

    QApplication::setPalette(qpal);
    if (auto *app = qobject_cast<QApplication*>(QApplication::instance())) {
        app->setStyleSheet(getStyleSheet(theme));
    }
}
