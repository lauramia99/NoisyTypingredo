#ifndef VERIFICATIONSTATSCSVWRITER_H
#define VERIFICATIONSTATSCSVWRITER_H

#include "databasemanager.h"

#include <QString>

namespace VerificationStatsCsvWriter
{
QString defaultFilePath();

bool appendRow(const QString &filePath,
               const QString &participantId,
               const VerificationStats &stats);
}

#endif // VERIFICATIONSTATSCSVWRITER_H
