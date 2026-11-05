#include "logger.h"
#include <QDir>

Logger::Logger(const QString &logFilePath)
{
    QDir().mkpath(QFileInfo(logFilePath).absolutePath());
    m_file.setFileName(logFilePath);
    m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

Logger::~Logger()
{
    if (m_file.isOpen()) m_file.close();
}

void Logger::log(const QString &message)
{
    QMutexLocker lock(&m_mutex);
    if (!m_file.isOpen()) return;
    QTextStream out(&m_file);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "  " << message << "\n";
    out.flush();
}
