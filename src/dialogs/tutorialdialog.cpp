#include "tutorialdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QFile>
#include <QFileInfo>

TutorialDialog::TutorialDialog(QWidget *parent)
    : TutorialDialog(QString(), parent)
{
}

TutorialDialog::TutorialDialog(const QString &tutorialFile, QWidget *parent)
    : QDialog(parent)
{
    QString title = tutorialFile.isEmpty() ? "Getting Started with ROM Customization Toolkit" : QFileInfo(tutorialFile).completeBaseName();
    setWindowTitle("Tutorial – " + title);
    resize(820, 500);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *titleLabel = new QLabel(title);
    QFont f = titleLabel->font();
    f.setPointSize(14);
    titleLabel->setFont(f);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFixedHeight(36);
    titleLabel->setStyleSheet("background:#333; color: #fff; border-bottom:1px solid #555;");
    layout->addWidget(titleLabel);

    auto *textBrowser = new QTextBrowser;
    textBrowser->setReadOnly(true);
    textBrowser->setFrameShape(QFrame::NoFrame);
    textBrowser->setOpenExternalLinks(true);

    if (!tutorialFile.isEmpty() && QFile::exists(tutorialFile)) {
        QFile file(tutorialFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            textBrowser->setPlainText(QString::fromUtf8(file.readAll()));
    } else {
        textBrowser->setHtml(
            "<h2>ROM Customization Toolkit Linux Quickstart Guide</h2>"
            "<p><b>1. Unpacking Firmware:</b></p>"
            "<ul>"
            "<li>Click <i>'Unpack Image / Package'</i> and select an Amlogic, Rockchip, or Allwinner .img, .zip, or .ctp file.</li>"
            "<li>Select desired unpack levels in the partition selection dialog.</li>"
            "</ul>"
            "<p><b>2. Modifying Partitions & Apps:</b></p>"
            "<ul>"
            "<li>Use the <b>Apps</b> tab to add or remove APKs from system/vendor/product partitions.</li>"
            "<li>Use the <b>Boot Media</b> tab to preview and replace boot logo (.bmp), boot animation (.zip), and boot video.</li>"
            "<li>Use the <b>DTB Editor</b> to view, search, and edit Device Tree Source (.dts) files.</li>"
            "<li>Use the <b>Filesystem</b> tab to browse files, edit build.prop, keymaps, and inject root.</li>"
            "</ul>"
            "<p><b>3. Repacking:</b></p>"
            "<ul>"
            "<li>Go to the <b>Pack / Build</b> tab and click <i>'Repack to Image (.img)'</i> to generate a flashable burn image.</li>"
            "</ul>"
        );
    }

    layout->addWidget(textBrowser);
}
