#pragma once
#include <QString>
#include <QByteArray>

// Simple MD5 helper (replaces C# EasyMD5)
class EasyMD5 {
public:
    static QString hash(const QString &input);
    static QString hashBytes(const QByteArray &data);
};
