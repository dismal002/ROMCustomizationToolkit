#pragma once
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

/**
 * DownloadDialog – ports Form4.cs
 * Shows progress while downloading a file. Optionally extracts zip after download.
 */
class DownloadDialog : public QDialog {
    Q_OBJECT
public:
    explicit DownloadDialog(const QString &url, const QString &savePath,
                             QWidget *parent = nullptr);
    ~DownloadDialog();

private slots:
    void startDownload();
    void onProgress(qint64 received, qint64 total);
    void onFinished();

private:
    QString              m_url;
    QString              m_savePath;
    QProgressBar        *m_progress;
    QLabel              *m_label;
    QNetworkAccessManager m_mgr;
    QNetworkReply       *m_reply = nullptr;
    QFile                m_file;

    void extractZip(const QString &zipPath);
};
