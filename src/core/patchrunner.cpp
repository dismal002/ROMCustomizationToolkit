#include "patchrunner.h"
#include "platform.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDateTime>

PatchRunner::PatchRunner(Logger *logger, QObject *parent)
    : QObject(parent), m_logger(logger), m_shell(logger)
{}

// ---------------------------------------------------------------------------
// Metadata helpers
// ---------------------------------------------------------------------------

QString PatchRunner::readField(const QString &patchFile, const QString &key) const
{
    QFile f(patchFile);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&f);
    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith(key + "="))
            return line.mid(key.length() + 1);
    }
    return {};
}

QString PatchRunner::patchAuthor(const QString &patchFile) const
    { return readField(patchFile, "AUTHOR"); }
QString PatchRunner::patchVersion(const QString &patchFile) const
    { return readField(patchFile, "VERSION"); }
QString PatchRunner::patchDescription(const QString &patchFile) const
    { return readField(patchFile, "DESCRIPTION"); }

// ---------------------------------------------------------------------------
// Main patch runner
// ---------------------------------------------------------------------------

void PatchRunner::appendOutput(const QString &text)
{
    // Normalise path separators like original
    QString cleaned = text;
    cleaned.replace(m_rootDir, "");
    cleaned.replace('\\', '/');
    m_output += cleaned;
    emit outputAppended(cleaned);
}

bool PatchRunner::runPatch(const QString &patchFile, const QString &rootDirectory)
{
    m_logger->log(m_logHead + "Running patch: " + QFileInfo(patchFile).baseName());
    auto t0 = QDateTime::currentMSecsSinceEpoch();

    m_output  = "Running patch: " + QFileInfo(patchFile).baseName() + "..\n\n";
    m_rootDir = rootDirectory;
    if (!m_rootDir.endsWith('/')) m_rootDir += '/';

    QFile f(patchFile);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    bool result = true;
    bool ifvar  = true;

    QTextStream in(&f);

    in.setCodec("UTF-8");
    while (!in.atEnd()) {
        QString raw = in.readLine().replace("\r", "");
        if (raw.startsWith('#') || raw.trimmed().isEmpty()) continue;

        QStringList p = raw.split('|');
        QString cmd = p.value(0);

        // --- Conditionals (always evaluated) ---
        if (cmd == "IfDirExists")
            { ifvar = condDirExists(m_rootDir + p.value(1)); continue; }
        if (cmd == "IfFileExists")
            { ifvar = condFileExists(m_rootDir + p.value(1)); continue; }
        if (cmd == "IfDirNotExists")
            { ifvar = !condDirExists(m_rootDir + p.value(1)); continue; }
        if (cmd == "IfFileNotExists")
            { ifvar = !condFileExists(m_rootDir + p.value(1)); continue; }
        if (cmd == "IfContains")
            { ifvar = condContains(m_rootDir + p.value(1), p.value(2)); continue; }
        if (cmd == "IfNotContains")
            { ifvar = !condContains(m_rootDir + p.value(1), p.value(2)); continue; }

        if (!ifvar) { ifvar = true; continue; }

        // --- Script termination ---
        if (cmd == "Platform" && !checkPlatform(p.value(1))) {
            result = false;
            appendOutput("Incorrect platform.\nThis script is designed for " + p.value(1));
            break;
        }
        if (cmd == "EndScript") {
            result = (p.value(1) == "SUCCESS");
            appendOutput(p.value(2));
            break;
        }

        // --- Action commands ---
        if (cmd == "WriteLine")         appendOutput(p.value(1) + "\n");
        if (cmd == "SearchDeleteFile")  cmdSearchDeleteFile(m_rootDir + p.value(1), p.value(2));
        if (cmd == "SearchDeleteDir")   cmdSearchDeleteDir(m_rootDir + p.value(1), p.value(2));
        if (cmd == "DeleteFile")        cmdDeleteFile(m_rootDir + p.value(1));
        if (cmd == "DeleteDir")         cmdDeleteDir(m_rootDir + p.value(1));
        if (cmd == "CopyFile")          cmdCopyFile(m_rootDir + p.value(1),
                                                    m_rootDir + p.value(2),
                                                    p.value(3) == "true");
        if (cmd == "CopyDir")           cmdCopyDir(m_rootDir + p.value(1),
                                                   m_rootDir + p.value(2),
                                                   p.value(3) == "true");
        if (cmd == "MoveFile")          cmdMoveFile(m_rootDir + p.value(1),
                                                    m_rootDir + p.value(2),
                                                    p.value(3) == "true");
        if (cmd == "MoveDir")           cmdMoveDir(m_rootDir + p.value(1),
                                                   m_rootDir + p.value(2),
                                                   p.value(3) == "true");
        if (cmd == "CreateFile")        cmdCreateFile(m_rootDir + p.value(1));
        if (cmd == "CreateDir")         cmdCreateDir(m_rootDir + p.value(1));
        if (cmd == "AppendFile")        cmdAppendText(m_rootDir + p.value(1), p.value(2));
        if (cmd == "ReplaceLine")       cmdReplaceLine(m_rootDir + p.value(1),
                                                       p.value(2), p.value(3), p.value(4));
        if (cmd == "ReplaceText")       cmdReplaceText(m_rootDir + p.value(1),
                                                       p.value(2), p.value(3));
        if (cmd == "Download")          cmdDownload(m_rootDir + p.value(1),
                                                    p.value(2), p.value(3));
        if (cmd == "Unzip")             cmdUnzip(m_rootDir + p.value(1),
                                                 m_rootDir + p.value(2));
    }

    qint64 ms = QDateTime::currentMSecsSinceEpoch() - t0;
    appendOutput(QString("\nFinished running patch in %1ms").arg(ms));
    return result;
}

bool PatchRunner::checkPlatform(const QString &platform) const
{
    return platform.trimmed().toLower() == imageType.toLower();
}

// ---------------------------------------------------------------------------
// Condition helpers
// ---------------------------------------------------------------------------

bool PatchRunner::condDirExists(const QString &path) const
    { return QDir(path).exists(); }
bool PatchRunner::condFileExists(const QString &path) const
    { return QFile::exists(path); }
bool PatchRunner::condContains(const QString &file, const QString &phrase) const
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    return f.readAll().contains(phrase.toUtf8());
}

// ---------------------------------------------------------------------------
// Command implementations
// ---------------------------------------------------------------------------

void PatchRunner::cmdSearchDeleteFile(const QString &dir, const QString &pattern)
{
    m_logger->log(m_logHead + "SearchDeleteFile " + dir + " / " + pattern);
    QDirIterator it(dir, {pattern}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString f = it.next();
        QFile::remove(f);
        appendOutput("Deleted file " + f + "\n");
    }
}

void PatchRunner::cmdSearchDeleteDir(const QString &dir, const QString &pattern)
{
    m_logger->log(m_logHead + "SearchDeleteDir " + dir + " / " + pattern);
    QDirIterator it(dir, {pattern}, QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString d = it.next();
        ShellRunner::deleteDirectory(d);
        appendOutput("Deleted directory " + d + "\n");
    }
}

void PatchRunner::cmdDeleteFile(const QString &path)
{
    if (QFile::exists(path)) {
        QFile::remove(path);
        appendOutput("Deleted file " + path + "\n");
    }
}

void PatchRunner::cmdDeleteDir(const QString &path)
{
    if (QDir(path).exists()) {
        ShellRunner::deleteDirectory(path);
        appendOutput("Deleted directory " + path + "\n");
    }
}

void PatchRunner::cmdCopyFile(const QString &src, const QString &dst, bool overwrite)
{
    QDir().mkpath(QFileInfo(dst).absolutePath());
    if (overwrite && QFile::exists(dst)) QFile::remove(dst);
    QFile::copy(src, dst);
    appendOutput("Copied " + src + " → " + dst + "\n");
}

void PatchRunner::cmdCopyDir(const QString &src, const QString &dst, bool overwrite)
{
    Q_UNUSED(overwrite)
    QDirIterator it(src, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        QString rel = fi.absoluteFilePath().mid(src.length());
        QString target = dst + rel;
        if (fi.isDir()) QDir().mkpath(target);
        else { QDir().mkpath(QFileInfo(target).absolutePath()); QFile::copy(fi.absoluteFilePath(), target); }
    }
}

void PatchRunner::cmdMoveFile(const QString &src, const QString &dst, bool overwrite)
{
    QDir().mkpath(QFileInfo(dst).absolutePath());
    if (overwrite && QFile::exists(dst)) QFile::remove(dst);
    QFile::rename(src, dst);
    appendOutput("Moved " + src + " → " + dst + "\n");
}

void PatchRunner::cmdMoveDir(const QString &src, const QString &dst, bool /*overwrite*/)
{
    cmdCopyDir(src, dst, true);
    ShellRunner::deleteDirectory(src);
    appendOutput("Moved dir " + src + " → " + dst + "\n");
}

void PatchRunner::cmdCreateFile(const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.close();
}

void PatchRunner::cmdCreateDir(const QString &path)
{
    QDir().mkpath(path);
    appendOutput("Created dir " + path + "\n");
}

void PatchRunner::cmdAppendText(const QString &file, const QString &text)
{
    QFile f(file);
    if (f.open(QIODevice::Append)) {
        QTextStream out(&f);
        out.setCodec("UTF-8");
        out << text;
    }
}

void PatchRunner::cmdReplaceLine(const QString &file, const QString &lineStart,
                                  const QString &newText, const QString &mode)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QStringList lines;
    QTextStream in(&f);
    in.setCodec("UTF-8");
    while (!in.atEnd()) lines << in.readLine();
    f.close();

    bool replaced = false;
    for (QString &line : lines) {
        if (line.trimmed().startsWith(lineStart.trimmed())) {
            if (mode == "replace" || mode.isEmpty()) line = newText;
            else if (mode == "insertbefore") { line = newText + "\n" + line; }
            else if (mode == "insertafter")  { line = line + "\n" + newText; }
            replaced = true;
        }
    }

    if (f.open(QIODevice::WriteOnly)) {
        QTextStream out(&f);
        out.setCodec("UTF-8");
        out << lines.join('\n');
    }

    if (replaced)
        appendOutput("Replaced line in " + file + "\n");
}

void PatchRunner::cmdReplaceText(const QString &file, const QString &search,
                                  const QString &replace)
{
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray data = f.readAll();
    f.close();

    data.replace(search.toUtf8(), replace.toUtf8());

    if (f.open(QIODevice::WriteOnly)) f.write(data);
    appendOutput("Replaced text in " + file + "\n");
}

void PatchRunner::cmdDownload(const QString &localPath, const QString &url,
                               const QString &label)
{
    Q_UNUSED(label)
    m_logger->log(m_logHead + "Downloading " + url + " → " + localPath);
    QNetworkAccessManager mgr;
    QNetworkReply *reply = mgr.get(QNetworkRequest(QUrl(url)));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError) {
        QDir().mkpath(QFileInfo(localPath).absolutePath());
        QFile f(localPath);
        if (f.open(QIODevice::WriteOnly)) f.write(reply->readAll());
    }
    reply->deleteLater();
}

void PatchRunner::cmdUnzip(const QString &zipFile, const QString &outDir)
{
    QDir().mkpath(outDir);
    m_shell.run(Platform::sevenZip(), QString("x \"%1\" -o\"%2\" -y").arg(zipFile, outDir));
}
