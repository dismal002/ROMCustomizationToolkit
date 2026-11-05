#include "shellrunner.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

ShellRunner::ShellRunner(Logger *logger)
    : m_logger(logger)
{}

QString ShellRunner::run(const QString &executable, const QString &args,
                         const QString &workDir)
{
    return run(executable, QProcess::splitCommand(args), workDir);
}

QString ShellRunner::run(const QString &executable, const QStringList &args,
                         const QString &workDir)
{
    if (m_logger)
        m_logger->log(QString("[SHELL] %1 %2").arg(QFileInfo(executable).baseName(), args.join(" ")));

    QProcess proc;
    if (!workDir.isEmpty())
        proc.setWorkingDirectory(workDir);
    proc.setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_WIN
    // The app is a GUI program: don't flash a console window for every tool we launch.
    proc.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *a) {
        a->flags |= CREATE_NO_WINDOW;
    });
#endif
    proc.start(executable, args);
    proc.waitForFinished(-1);  // Wait indefinitely

    m_lastOutput = QString::fromLocal8Bit(proc.readAll());
    m_lastExitCode = (proc.exitStatus() == QProcess::NormalExit) ? proc.exitCode() : -1;

    if (m_logger && proc.error() == QProcess::FailedToStart)
        m_logger->log(QString("[SHELL] failed to start %1: %2").arg(executable, proc.errorString()));
    if (m_logger && !m_lastOutput.trimmed().isEmpty())
        m_logger->log(QString("[STDOUT] %1").arg(m_lastOutput.left(500)));
    if (m_logger && m_lastExitCode != 0)
        m_logger->log(QString("[SHELL] %1 exited with code %2")
                          .arg(QFileInfo(executable).fileName()).arg(m_lastExitCode));

    return m_lastOutput;
}

int ShellRunner::runGetCode(const QString &executable, const QString &args,
                            const QString &workDir)
{
    return runGetCode(executable, QProcess::splitCommand(args), workDir);
}

int ShellRunner::runGetCode(const QString &executable, const QStringList &args,
                            const QString &workDir)
{
    run(executable, args, workDir);
    return m_lastExitCode;
}

bool ShellRunner::deleteDirectory(const QString &path)
{
    QDir dir(path);
    return dir.removeRecursively();
}
