#pragma once
#include <QString>
#include "logger.h"

/**
 * ShellRunner – synchronous/async process launcher.
 * Replaces the Thread + Application.DoEvents() pattern from the original C# code.
 * Uses QProcess under the hood; runs in a worker thread and blocks the caller.
 */
class ShellRunner {
public:
    explicit ShellRunner(Logger *logger = nullptr);

    // Run executable with args, block until done, return stdout+stderr.
    QString run(const QString &executable, const QString &args,
                const QString &workDir = QString());
    QString run(const QString &executable, const QStringList &args,
                const QString &workDir = QString());

    // Run and return exit code (output available via lastOutput()).
    int runGetCode(const QString &executable, const QString &args,
                   const QString &workDir = QString());
    int runGetCode(const QString &executable, const QStringList &args,
                   const QString &workDir = QString());

    QString lastOutput() const { return m_lastOutput; }
    int lastExitCode() const { return m_lastExitCode; }

    // Recursively delete a directory (replaces cmd.exe rmdir /S /Q).
    static bool deleteDirectory(const QString &path);

private:
    Logger  *m_logger;
    QString  m_lastOutput;
    int      m_lastExitCode = 0;
};
