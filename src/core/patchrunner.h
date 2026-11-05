#pragma once
#include <QString>
#include <QStringList>
#include "logger.h"
#include "shellrunner.h"

/**
 * PatchRunner – ports PatchClass.cs
 * Executes .patch script files which use a simple line-based DSL.
 *
 * Supported commands:
 *   Platform|<type>
 *   WriteLine|<text>
 *   SearchDeleteFile|<dir>|<pattern>
 *   SearchDeleteDir|<dir>|<pattern>
 *   DeleteFile|<path>
 *   DeleteDir|<path>
 *   CopyFile|<src>|<dst>|<overwrite>
 *   CopyDir|<src>|<dst>|<overwrite>
 *   MoveFile|<src>|<dst>|<overwrite>
 *   MoveDir|<src>|<dst>|<overwrite>
 *   CreateFile|<path>
 *   CreateDir|<path>
 *   AppendFile|<path>|<text>
 *   ReplaceLine|<file>|<linestart>|<newtext>|<mode>
 *   ReplaceText|<file>|<search>|<replace>
 *   Download|<localpath>|<url>|<label>
 *   Unzip|<zipfile>|<outdir>
 *   IfDirExists|<path>
 *   IfFileExists|<path>
 *   IfDirNotExists|<path>
 *   IfFileNotExists|<path>
 *   IfContains|<file>|<phrase>
 *   IfNotContains|<file>|<phrase>
 *   EndScript|SUCCESS|<msg>  /  EndScript|FAILED|<msg>
 */
class PatchRunner : public QObject {
    Q_OBJECT
public:
    explicit PatchRunner(Logger *logger, QObject *parent = nullptr);

    // Parse metadata from patch file header
    QString patchAuthor(const QString &patchFile) const;
    QString patchVersion(const QString &patchFile) const;
    QString patchDescription(const QString &patchFile) const;

    // Execute patch against rootDirectory
    bool runPatch(const QString &patchFile, const QString &rootDirectory);

    // For the patch output text panel
    QString patchOutput() const { return m_output; }

    // Image type string (e.g. "amlogic", "rockchip", "allwinner")
    QString imageType;

signals:
    void outputAppended(const QString &text);

private:
    Logger      *m_logger;
    ShellRunner  m_shell;
    QString      m_logHead = "[PATCHER] : ";
    QString      m_output;
    QString      m_rootDir;

    QString readField(const QString &patchFile, const QString &key) const;
    void appendOutput(const QString &text);

    // Command implementations
    void cmdSearchDeleteFile(const QString &dir, const QString &pattern);
    void cmdSearchDeleteDir(const QString &dir, const QString &pattern);
    void cmdDeleteFile(const QString &path);
    void cmdDeleteDir(const QString &path);
    void cmdCopyFile(const QString &src, const QString &dst, bool overwrite);
    void cmdCopyDir(const QString &src, const QString &dst, bool overwrite);
    void cmdMoveFile(const QString &src, const QString &dst, bool overwrite);
    void cmdMoveDir(const QString &src, const QString &dst, bool overwrite);
    void cmdCreateFile(const QString &path);
    void cmdCreateDir(const QString &path);
    void cmdAppendText(const QString &file, const QString &text);
    void cmdReplaceLine(const QString &file, const QString &lineStart,
                        const QString &newText, const QString &mode);
    void cmdReplaceText(const QString &file, const QString &search,
                        const QString &replace);
    void cmdDownload(const QString &localPath, const QString &url, const QString &label);
    void cmdUnzip(const QString &zipFile, const QString &outDir);

    bool condDirExists(const QString &path) const;
    bool condFileExists(const QString &path) const;
    bool condContains(const QString &file, const QString &phrase) const;
    bool checkPlatform(const QString &platform) const;
};
