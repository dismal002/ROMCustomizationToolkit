#include "iconhelper.h"

#include <QPainter>
#include <QPainterPath>
#include <QImageReader>
#include <QFile>
#include <QCoreApplication>
#include <QtMath>

static QString findSvgFile(IconHelper::IconType type)
{
    QString baseName;
    QString alias;
    switch (type) {
    case IconHelper::Home:       baseName = QStringLiteral("home_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("home.svg"); break;
    case IconHelper::NavHome:    baseName = QStringLiteral("ic_sysbar_home.png"); alias = QStringLiteral("home_nav.png"); break;
    case IconHelper::Apps:       baseName = QStringLiteral("apps_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("apps.svg"); break;
    case IconHelper::Display:    baseName = QStringLiteral("display_settings_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("display.svg"); break;
    case IconHelper::Battery:    baseName = QStringLiteral("battery_5_bar_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("battery.svg"); break;
    case IconHelper::Monitor:    baseName = QStringLiteral("monitoring_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("monitor.svg"); break;
    case IconHelper::Sliders:    baseName = QStringLiteral("format_color_fill_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("sliders.svg"); break;
    case IconHelper::Script:     baseName = QStringLiteral("description_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("script.svg"); break;
    case IconHelper::Terminal:   baseName = QStringLiteral("terminal_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("terminal.svg"); break;
    case IconHelper::Settings:   baseName = QStringLiteral("settings_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("settings.svg"); break;
    case IconHelper::Refresh:    baseName = QStringLiteral("refresh_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("refresh.svg"); break;
    case IconHelper::Check:      baseName = QStringLiteral("check_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("check.svg"); break;
    case IconHelper::Restore:    baseName = QStringLiteral("settings_backup_restore_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("restore.svg"); break;
    case IconHelper::Info:       baseName = QStringLiteral("info_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("info.svg"); break;
    case IconHelper::Power:      baseName = QStringLiteral("power_settings_new_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("power.svg"); break;
    case IconHelper::Lock:       baseName = QStringLiteral("lock_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("lock.svg"); break;
    case IconHelper::VolumeUp:   baseName = QStringLiteral("volume_up_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("volume_up.svg"); break;
    case IconHelper::VolumeDown: baseName = QStringLiteral("volume_down_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("volume_down.svg"); break;
    case IconHelper::Mute:       baseName = QStringLiteral("volume_mute_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("volume_mute.svg"); break;
    case IconHelper::Back:       baseName = QStringLiteral("ic_sysbar_back.png"); alias = QStringLiteral("back.png"); break;
    case IconHelper::Recents:    baseName = QStringLiteral("ic_sysbar_recent.png"); alias = QStringLiteral("recent.png"); break;
    case IconHelper::Screenshot: baseName = QStringLiteral("screenshot_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("screenshot.svg"); break;
    case IconHelper::Network:    baseName = QStringLiteral("wifi_24dp_000_FILL0_wght400_GRAD0_opsz24.svg"); alias = QStringLiteral("network.svg"); break;
    default: break;
    }

    if (baseName.isEmpty()) return QString();

    // 1. Check local icons/ directory on disk first
    const QStringList diskPaths = {
        QStringLiteral("icons/") + baseName,
        QCoreApplication::applicationDirPath() + QStringLiteral("/icons/") + baseName,
        QStringLiteral("../icons/") + baseName
    };
    for (const QString &p : diskPaths) {
        if (QFile::exists(p)) return p;
    }

    // 2. Check embedded resources
    if (QFile::exists(QStringLiteral(":/icons/") + alias)) {
        return QStringLiteral(":/icons/") + alias;
    }

    return QString();
}

QPixmap IconHelper::createPixmapFromSvg(const QString &svgPath, const QColor &color, int size)
{
    if (svgPath.isEmpty()) return QPixmap();

    QImage img;
    QImageReader reader(svgPath);
    if (reader.canRead()) {
        reader.setScaledSize(QSize(size, size));
        img = reader.read();
    }
    if (img.isNull()) {
        QIcon ic(svgPath);
        if (!ic.isNull()) {
            QPixmap px = ic.pixmap(size, size);
            if (!px.isNull()) img = px.toImage();
        }
    }

    if (img.isNull()) return QPixmap();

    QImage colored = img.convertToFormat(QImage::Format_ARGB32);
    QPainter p(&colored);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(colored.rect(), color);
    p.end();

    return QPixmap::fromImage(colored);
}

QIcon IconHelper::createIconFromSvg(const QString &svgPath, const QColor &normalColor, const QColor &selectedColor, int size)
{
    QIcon icon;
    QPixmap normalPx = createPixmapFromSvg(svgPath, normalColor, size);
    if (!normalPx.isNull()) {
        icon.addPixmap(normalPx, QIcon::Normal, QIcon::Off);
        icon.addPixmap(normalPx, QIcon::Normal, QIcon::On);
    }
    if (selectedColor.isValid()) {
        QPixmap selPx = createPixmapFromSvg(svgPath, selectedColor, size);
        if (!selPx.isNull()) {
            icon.addPixmap(selPx, QIcon::Selected, QIcon::On);
            icon.addPixmap(selPx, QIcon::Selected, QIcon::Off);
            icon.addPixmap(selPx, QIcon::Active, QIcon::On);
            icon.addPixmap(selPx, QIcon::Active, QIcon::Off);
        }
    }
    return icon;
}

QPixmap IconHelper::createPixmap(IconType type, const QColor &color, int size)
{
    QString svgFile = findSvgFile(type);
    if (!svgFile.isEmpty()) {
        QPixmap svgPix = createPixmapFromSvg(svgFile, color, size);
        if (!svgPix.isNull()) return svgPix;
    }

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPen pen(color);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    qreal s = static_cast<qreal>(size);
    pen.setWidthF(qMax(1.6, s / 16.0));
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    switch (type) {
    case Home: {
        // Roof
        QPainterPath path;
        path.moveTo(s * 0.15, s * 0.48);
        path.lineTo(s * 0.50, s * 0.18);
        path.lineTo(s * 0.85, s * 0.48);
        // Walls
        path.moveTo(s * 0.25, s * 0.45);
        path.lineTo(s * 0.25, s * 0.82);
        path.lineTo(s * 0.75, s * 0.82);
        path.lineTo(s * 0.75, s * 0.45);
        // Door
        path.moveTo(s * 0.42, s * 0.82);
        path.lineTo(s * 0.42, s * 0.58);
        path.lineTo(s * 0.58, s * 0.58);
        path.lineTo(s * 0.58, s * 0.82);
        p.drawPath(path);
        break;
    }
    case Apps: {
        // 4 small rounded rects
        qreal boxSize = s * 0.26;
        qreal r = s * 0.06;
        p.setBrush(color);
        p.drawRoundedRect(QRectF(s * 0.18, s * 0.18, boxSize, boxSize), r, r);
        p.drawRoundedRect(QRectF(s * 0.56, s * 0.18, boxSize, boxSize), r, r);
        p.drawRoundedRect(QRectF(s * 0.18, s * 0.56, boxSize, boxSize), r, r);
        p.drawRoundedRect(QRectF(s * 0.56, s * 0.56, boxSize, boxSize), r, r);
        break;
    }
    case Display: {
        // Monitor screen
        p.drawRoundedRect(QRectF(s * 0.14, s * 0.20, s * 0.72, s * 0.50), s * 0.06, s * 0.06);
        // Stand
        p.drawLine(QPointF(s * 0.50, s * 0.70), QPointF(s * 0.50, s * 0.84));
        p.drawLine(QPointF(s * 0.32, s * 0.84), QPointF(s * 0.68, s * 0.84));
        break;
    }
    case Sliders: {
        // 3 slider lines with handles
        // Slider 1
        p.drawLine(QPointF(s * 0.25, s * 0.18), QPointF(s * 0.25, s * 0.82));
        p.setBrush(color);
        p.drawEllipse(QPointF(s * 0.25, s * 0.40), s * 0.08, s * 0.08);

        // Slider 2
        p.drawLine(QPointF(s * 0.50, s * 0.18), QPointF(s * 0.50, s * 0.82));
        p.drawEllipse(QPointF(s * 0.50, s * 0.62), s * 0.08, s * 0.08);

        // Slider 3
        p.drawLine(QPointF(s * 0.75, s * 0.18), QPointF(s * 0.75, s * 0.82));
        p.drawEllipse(QPointF(s * 0.75, s * 0.30), s * 0.08, s * 0.08);
        break;
    }
    case Battery: {
        // Battery body
        p.drawRoundedRect(QRectF(s * 0.14, s * 0.30, s * 0.64, s * 0.40), s * 0.06, s * 0.06);
        // Battery tip
        p.setBrush(color);
        p.drawRoundedRect(QRectF(s * 0.78, s * 0.42, s * 0.08, s * 0.16), s * 0.02, s * 0.02);
        // Charge level fill
        p.drawRoundedRect(QRectF(s * 0.20, s * 0.36, s * 0.35, s * 0.28), s * 0.03, s * 0.03);
        break;
    }
    case Monitor: {
        // Pulse wave line
        QPainterPath path;
        path.moveTo(s * 0.10, s * 0.52);
        path.lineTo(s * 0.30, s * 0.52);
        path.lineTo(s * 0.40, s * 0.22);
        path.lineTo(s * 0.55, s * 0.78);
        path.lineTo(s * 0.65, s * 0.42);
        path.lineTo(s * 0.72, s * 0.52);
        path.lineTo(s * 0.90, s * 0.52);
        p.drawPath(path);
        break;
    }
    case Script: {
        // Document with folded corner
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.18);
        path.lineTo(s * 0.60, s * 0.18);
        path.lineTo(s * 0.78, s * 0.36);
        path.lineTo(s * 0.78, s * 0.84);
        path.lineTo(s * 0.22, s * 0.84);
        path.closeSubpath();
        path.moveTo(s * 0.60, s * 0.18);
        path.lineTo(s * 0.60, s * 0.36);
        path.lineTo(s * 0.78, s * 0.36);
        p.drawPath(path);
        // Code lines
        p.drawLine(QPointF(s * 0.32, s * 0.50), QPointF(s * 0.68, s * 0.50));
        p.drawLine(QPointF(s * 0.32, s * 0.62), QPointF(s * 0.58, s * 0.62));
        p.drawLine(QPointF(s * 0.32, s * 0.74), QPointF(s * 0.64, s * 0.74));
        break;
    }
    case Terminal: {
        // Window box
        p.drawRoundedRect(QRectF(s * 0.14, s * 0.20, s * 0.72, s * 0.60), s * 0.08, s * 0.08);
        // Prompt >
        QPainterPath path;
        path.moveTo(s * 0.28, s * 0.40);
        path.lineTo(s * 0.42, s * 0.50);
        path.lineTo(s * 0.28, s * 0.60);
        p.drawPath(path);
        // Cursor _
        p.drawLine(QPointF(s * 0.48, s * 0.62), QPointF(s * 0.68, s * 0.62));
        break;
    }
    case Settings: {
        // Central circle
        p.drawEllipse(QPointF(s * 0.50, s * 0.50), s * 0.16, s * 0.16);
        // 8 gear teeth
        for (int i = 0; i < 8; ++i) {
            qreal angle = i * (M_PI / 4.0);
            qreal cosA = qCos(angle);
            qreal sinA = qSin(angle);
            QPointF p1(s * 0.50 + cosA * (s * 0.22), s * 0.50 + sinA * (s * 0.22));
            QPointF p2(s * 0.50 + cosA * (s * 0.36), s * 0.50 + sinA * (s * 0.36));
            p.drawLine(p1, p2);
        }
        break;
    }
    case Refresh: {
        // Circular arrow
        QRectF rect(s * 0.20, s * 0.20, s * 0.60, s * 0.60);
        p.drawArc(rect, 45 * 16, 270 * 16);
        // Arrow head
        qreal ax = s * 0.50 + qCos(qDegreesToRadians(45.0)) * (s * 0.30);
        qreal ay = s * 0.50 - qSin(qDegreesToRadians(45.0)) * (s * 0.30);
        QPainterPath arrow;
        arrow.moveTo(ax, ay);
        arrow.lineTo(ax + s * 0.08, ay - s * 0.08);
        arrow.moveTo(ax, ay);
        arrow.lineTo(ax - s * 0.08, ay - s * 0.08);
        p.drawPath(arrow);
        break;
    }
    case Network: {
        // WiFi / LAN arcs
        QRectF r1(s * 0.15, s * 0.20, s * 0.70, s * 0.70);
        QRectF r2(s * 0.28, s * 0.35, s * 0.44, s * 0.44);
        p.drawArc(r1, 45 * 16, 90 * 16);
        p.drawArc(r2, 45 * 16, 90 * 16);
        p.setBrush(color);
        p.drawEllipse(QPointF(s * 0.50, s * 0.75), s * 0.06, s * 0.06);
        break;
    }
    case Back: {
        QPainterPath path;
        path.moveTo(s * 0.62, s * 0.25);
        path.lineTo(s * 0.38, s * 0.50);
        path.lineTo(s * 0.62, s * 0.75);
        p.drawPath(path);
        break;
    }
    case Recents: {
        p.drawRoundedRect(QRectF(s * 0.24, s * 0.24, s * 0.52, s * 0.52), s * 0.08, s * 0.08);
        break;
    }
    case Lock: {
        // Shackle
        QRectF arcRect(s * 0.30, s * 0.18, s * 0.40, s * 0.36);
        p.drawArc(arcRect, 0, 180 * 16);
        // Body
        p.drawRoundedRect(QRectF(s * 0.25, s * 0.36, s * 0.50, s * 0.46), s * 0.08, s * 0.08);
        // Keyhole
        p.setBrush(color);
        p.drawEllipse(QPointF(s * 0.50, s * 0.54), s * 0.05, s * 0.05);
        p.drawLine(QPointF(s * 0.50, s * 0.56), QPointF(s * 0.50, s * 0.68));
        break;
    }
    case VolumeUp: {
        // Speaker
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.40);
        path.lineTo(s * 0.36, s * 0.40);
        path.lineTo(s * 0.50, s * 0.26);
        path.lineTo(s * 0.50, s * 0.74);
        path.lineTo(s * 0.36, s * 0.60);
        path.lineTo(s * 0.22, s * 0.60);
        path.closeSubpath();
        p.setBrush(color);
        p.drawPath(path);
        p.setBrush(Qt::NoBrush);
        // Sound waves
        QRectF arc1(s * 0.42, s * 0.36, s * 0.28, s * 0.28);
        QRectF arc2(s * 0.38, s * 0.26, s * 0.44, s * 0.44);
        p.drawArc(arc1, -55 * 16, 110 * 16);
        p.drawArc(arc2, -55 * 16, 110 * 16);
        break;
    }
    case VolumeDown: {
        // Speaker
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.40);
        path.lineTo(s * 0.36, s * 0.40);
        path.lineTo(s * 0.50, s * 0.26);
        path.lineTo(s * 0.50, s * 0.74);
        path.lineTo(s * 0.36, s * 0.60);
        path.lineTo(s * 0.22, s * 0.60);
        path.closeSubpath();
        p.setBrush(color);
        p.drawPath(path);
        p.setBrush(Qt::NoBrush);
        // 1 wave
        QRectF arc(s * 0.42, s * 0.36, s * 0.28, s * 0.28);
        p.drawArc(arc, -55 * 16, 110 * 16);
        break;
    }
    case Mute: {
        // Speaker
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.40);
        path.lineTo(s * 0.36, s * 0.40);
        path.lineTo(s * 0.50, s * 0.26);
        path.lineTo(s * 0.50, s * 0.74);
        path.lineTo(s * 0.36, s * 0.60);
        path.lineTo(s * 0.22, s * 0.60);
        path.closeSubpath();
        p.setBrush(color);
        p.drawPath(path);
        p.setBrush(Qt::NoBrush);
        // X
        p.drawLine(QPointF(s * 0.62, s * 0.38), QPointF(s * 0.82, s * 0.62));
        p.drawLine(QPointF(s * 0.82, s * 0.38), QPointF(s * 0.62, s * 0.62));
        break;
    }
    case Screenshot: {
        // Phone body
        p.drawRoundedRect(QRectF(s * 0.28, s * 0.16, s * 0.44, s * 0.68), s * 0.08, s * 0.08);
        // Screen border
        p.drawRect(QRectF(s * 0.33, s * 0.26, s * 0.34, s * 0.46));
        // Home button
        p.drawPoint(QPointF(s * 0.50, s * 0.78));
        break;
    }
    case Check: {
        QPainterPath path;
        path.moveTo(s * 0.22, s * 0.52);
        path.lineTo(s * 0.42, s * 0.72);
        path.lineTo(s * 0.78, s * 0.28);
        p.drawPath(path);
        break;
    }
    case Restore: {
        QRectF rect(s * 0.20, s * 0.20, s * 0.60, s * 0.60);
        p.drawArc(rect, 45 * 16, 270 * 16);
        // Arrow pointing counter-clockwise
        qreal ax = s * 0.50 - qCos(qDegreesToRadians(45.0)) * (s * 0.30);
        qreal ay = s * 0.50 - qSin(qDegreesToRadians(45.0)) * (s * 0.30);
        QPainterPath arrow;
        arrow.moveTo(ax, ay);
        arrow.lineTo(ax - s * 0.08, ay - s * 0.08);
        arrow.moveTo(ax, ay);
        arrow.lineTo(ax + s * 0.08, ay - s * 0.08);
        p.drawPath(arrow);
        break;
    }
    case Info: {
        p.drawEllipse(QRectF(s * 0.18, s * 0.18, s * 0.64, s * 0.64));
        p.setBrush(color);
        p.drawEllipse(QPointF(s * 0.50, s * 0.36), s * 0.04, s * 0.04);
        p.drawLine(QPointF(s * 0.50, s * 0.46), QPointF(s * 0.50, s * 0.66));
        break;
    }
    case Power: {
        QRectF arcRect(s * 0.22, s * 0.22, s * 0.56, s * 0.56);
        p.drawArc(arcRect, 30 * 16, 300 * 16);
        p.drawLine(QPointF(s * 0.50, s * 0.14), QPointF(s * 0.50, s * 0.50));
        break;
    }
    }

    return pixmap;
}

QIcon IconHelper::createIcon(IconType type, const QColor &normalColor, const QColor &selectedColor, int size)
{
    QIcon icon;
    icon.addPixmap(createPixmap(type, normalColor, size), QIcon::Normal, QIcon::Off);
    if (selectedColor.isValid()) {
        icon.addPixmap(createPixmap(type, selectedColor, size), QIcon::Selected, QIcon::On);
        icon.addPixmap(createPixmap(type, selectedColor, size), QIcon::Selected, QIcon::Off);
        icon.addPixmap(createPixmap(type, selectedColor, size), QIcon::Active, QIcon::On);
        icon.addPixmap(createPixmap(type, selectedColor, size), QIcon::Active, QIcon::Off);
    }
    return icon;
}
