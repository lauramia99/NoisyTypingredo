#ifndef AUDITEVENTSCSVWRITER_H
#define AUDITEVENTSCSVWRITER_H

#include "databasemanager.h"

#include <QString>
#include <QVector>

namespace AuditEventsCsvWriter
{
QString defaultFilePath();
bool writeRows(const QString &filePath,
               const QVector<AuditEventExportRow> &rows);
}

#endif // AUDITEVENTSCSVWRITER_H