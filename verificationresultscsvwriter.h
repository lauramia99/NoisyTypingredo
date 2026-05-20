#ifndef VERIFICATIONRESULTSCSVWRITER_H
#define VERIFICATIONRESULTSCSVWRITER_H

#include "databasemanager.h"

#include <QString>
#include <QVector>

namespace VerificationResultsCsvWriter
{
QString defaultFilePath();

bool writeRows(const QString &filePath,
               const QVector<VerificationResultExportRow> &rows);
}

#endif // VERIFICATIONRESULTSCSVWRITER_H
