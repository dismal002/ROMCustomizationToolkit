#include "downloaddialog.h"
#include "../core/platform.h"
#include <QVBoxLayout>
#include <QTimer>
#include <QProcess>
#include <QFileInfo>
#include <QStandardPaths>

DownloadDialog::DownloadDialog(const QString &url, const QString &savePath, QWidget *parent)
    : QDialog(parent), m_url(url), m_savePath(savePath)
{
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    setFixedSize(340, 80);

    QString name = QFileInfo(savePath).completeBaseName();
    setWindowTitle(savePath.endsWith(".zip") ? "Downloading " + name : "Downloading update");

    auto *layout = new QVBoxLayout(this);
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_label = new QLabel("Connecting to server..");
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_progress);
    layout->addWidget(m_label);

    // Start download after dialog is shown
    QTimer::singleShot(0, this, &DownloadDialog::startDownload);
}

DownloadDialog::~DownloadDialog()
{
    if (m_reply) m_reply->abort();
}

void DownloadDialog::startDownload()
{
    m_file.setFileName(m_savePath);
    if (!m_file.open(QIODevice::WriteOnly)) {
        m_label->setText("Failed to open output file");
        return;
    }

    QNetworkRequest req;
    req.setUrl(QUrl(m_url));
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "Mozilla/5.0 (X11; Linux x86_64) ROM Customization Toolkit/1.0");
    m_reply = m_mgr.get(req);

    connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadDialog::onProgress);
    connect(m_reply, &QNetworkReply::finished,          this, &DownloadDialog::onFinished);
    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_file.isOpen()) m_file.write(m_reply->readAll());
    });
}

void DownloadDialog::onProgress(qint64 received, qint64 total)
{
    if (total <= 0) return;
    int pct = static_cast<int>(received * 100 / total);
    m_progress->setValue(pct);
    m_label->setText(QString("Downloaded %1 / %2 MB")
                     .arg(received / 1024 / 1024)
                     .arg(total   / 1024 / 1024));
}

void DownloadDialog::onFinished()
{
    m_file.close();

    if (m_reply->error() != QNetworkReply::NoError) {
        m_label->setText("Download failed: " + m_reply->errorString());
        m_reply->deleteLater();
        return;
    }

    m_reply->deleteLater();

    if (m_savePath.endsWith(".zip", Qt::CaseInsensitive)) {
        m_label->setText("Unpacking " + QFileInfo(m_savePath).completeBaseName() + "..");
        extractZip(m_savePath);
        QFile::remove(m_savePath);
    }

    m_label->setText("Completed");
    accept();
}

void DownloadDialog::extractZip(const QString &zipPath)
{
    // Bundled / system 7-Zip (7z, 7za, or 7-Zip's install folder on Windows)
    const QString tool = Platform::findSevenZip();
    if (tool.isEmpty()) return;

    QString outDir = QFileInfo(zipPath).absolutePath();
    QProcess proc;
    proc.start(tool, {"x", zipPath, "-o" + outDir, "-y"});
    proc.waitForFinished(-1);
}
