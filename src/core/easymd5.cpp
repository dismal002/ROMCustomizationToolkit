#include "easymd5.h"
#include <QCryptographicHash>

QString EasyMD5::hash(const QString &input)
{
    return hashBytes(input.toUtf8());
}

QString EasyMD5::hashBytes(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}
