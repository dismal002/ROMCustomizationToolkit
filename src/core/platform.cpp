#include "platform.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>   // used by dirIsWritable() on Windows

namespace Platform {

namespace {

#ifdef Q_OS_WIN
bool dirIsWritable(const QString &dir)
{
    QDir().mkpath(dir);
    QTemporaryFile probe(dir + "/.write_test_XXXXXX");
    return probe.open();
}
#endif

void copyMissing(const QString &src, const QString &dst)
{
    QDir sd(src);
    if (!sd.exists()) return;
    QDir().mkpath(dst);
    const auto entries = sd.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &fi : entries) {
        const QString target = dst + "/" + fi.fileName();
        if (fi.isDir())
            copyMissing(fi.absoluteFilePath(), target);
        else if (!QFile::exists(target))
            QFile::copy(fi.absoluteFilePath(), target);
    }
}

QStringList exeSuffixes()
{
    if (isWindows) return {"", ".exe", ".bat", ".cmd"};
    return {""};
}

} // namespace

QString rootDir()
{
    static const QString root = []() -> QString {
        const QByteArray env = qgetenv("ROMTOOLKIT_HOME");
        if (!env.isEmpty()) {
            const QString p = QDir(QString::fromLocal8Bit(env)).absolutePath();
            QDir().mkpath(p);
            return p;
        }
#ifdef Q_OS_WIN
        // Portable mode: keep everything next to the .exe if we are allowed to write there
        // (zip distribution). Installed under Program Files -> fall back to %APPDATA%.
        const QString app = QCoreApplication::applicationDirPath();
        if (dirIsWritable(app)) return app;
        const QString data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(data);
        return data;
#else
        return QDir::currentPath();   // legacy behaviour on Linux
#endif
    }();
    return root;
}

QString binDir()          { return rootDir() + "/bin"; }
QString tmpDir()          { return rootDir() + "/tmp"; }
QString logsDir()         { return rootDir() + "/logs"; }
QString patchesDir()      { return rootDir() + "/patches"; }
QString keysDir()         { return rootDir() + "/keys"; }
QString translationsDir() { return rootDir() + "/translations"; }

QString defaultSaveDir()
{
    if (!isWindows) return QDir::currentPath();
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return docs.isEmpty() ? rootDir() : docs;
}

void ensureLayout()
{
    for (const QString &d : {logsDir(), binDir(), patchesDir(), tmpDir(), keysDir(), translationsDir()})
        QDir().mkpath(d);

    // Installed read-only (e.g. Program Files): the shipped patches/keys live next to the exe,
    // the user-writable copies live in the data folder. Copy over what is missing.
    const QString app = QCoreApplication::applicationDirPath();
    if (QDir(app).absolutePath() != QDir(rootDir()).absolutePath()) {
        copyMissing(app + "/patches", patchesDir());
        copyMissing(app + "/keys", keysDir());
    }
}

QStringList binSearchDirs()
{
    QStringList dirs{binDir()};
    const QString appBin = QCoreApplication::applicationDirPath() + "/bin";
    if (QDir(appBin).absolutePath() != QDir(binDir()).absolutePath())
        dirs << appBin;
    return dirs;
}

QString findTool(const QString &name)
{
    for (const QString &dir : binSearchDirs()) {
        for (const QString &suffix : exeSuffixes()) {
            const QString candidate = dir + "/" + name + suffix;
            if (QFileInfo(candidate).isFile()) return candidate;
        }
    }
    return QStandardPaths::findExecutable(name);
}

QString binFile(const QString &relPath)
{
    for (const QString &dir : binSearchDirs()) {
        const QString candidate = dir + "/" + relPath;
        if (QFile::exists(candidate)) return candidate;
    }
    return binDir() + "/" + relPath;
}

QString findSevenZip()
{
    for (const QString &name : {QStringLiteral("7z"), QStringLiteral("7za"), QStringLiteral("7zz")}) {
        const QString p = findTool(name);
        if (!p.isEmpty()) return p;
    }
    if (isWindows) {
        for (const char *var : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
            const QString base = QString::fromLocal8Bit(qgetenv(var));
            if (base.isEmpty()) continue;
            const QString p = base + "/7-Zip/7z.exe";
            if (QFile::exists(p)) return p;
        }
    }
    return QString();
}

QString sevenZip()
{
    const QString p = findSevenZip();
    return p.isEmpty() ? QStringLiteral("7z") : p;
}

QString python()
{
    for (const QString &name : isWindows ? QStringList{"python3", "python"} : QStringList{"python3"}) {
        const QString p = findTool(name);
        if (!p.isEmpty()) return p;
    }
    return isWindows ? QStringLiteral("python") : QStringLiteral("python3");
}

QString bash()
{
    if (!isWindows) {
        const QString p = QStandardPaths::findExecutable("bash");
        return p.isEmpty() ? QStringLiteral("bash") : p;
    }
    // On Windows System32\bash.exe is the WSL launcher (different file system view), so only
    // accept a bundled bash or Git for Windows.
    const QString bundled = findTool("bash");
    if (!bundled.isEmpty() && !bundled.contains("system32", Qt::CaseInsensitive)) return bundled;
    for (const char *var : {"ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"}) {
        const QString base = QString::fromLocal8Bit(qgetenv(var));
        if (base.isEmpty()) continue;
        const QString p = base + "/Git/bin/bash.exe";
        if (QFile::exists(p)) return p;
    }
    return QString();
}

QString findScript(const QString &relPathNoExt)
{
    const QStringList order = isWindows ? QStringList{".bat", ".cmd", ".sh"}
                                        : QStringList{".sh"};
    for (const QString &dir : binSearchDirs())
        for (const QString &ext : order) {
            const QString candidate = dir + "/" + relPathNoExt + ext;
            if (QFile::exists(candidate)) return candidate;
        }
    return QString();
}

qint64 directorySize(const QString &dir)
{
    qint64 total = 0;
    QDirIterator it(dir, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

} // namespace Platform
