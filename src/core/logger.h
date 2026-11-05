#pragma once
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

class Logger {
public:
    explicit Logger(const QString &logFilePath);
    ~Logger();
    void log(const QString &message);

private:
    QFile   m_file;
    QMutex  m_mutex;
};
