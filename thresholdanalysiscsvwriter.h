#ifndef THRESHOLDANALYSISCSVWRITER_H
#define THRESHOLDANALYSISCSVWRITER_H

#include "thresholdanalysis.h"

#include <QString>
#include <QVector>

namespace ThresholdAnalysisCsvWriter
{
QString defaultFilePath();

bool writeRows(const QString &filePath,
               const QString &participantId,
               const QVector<ThresholdAnalysisRow> &rows);
}

#endif // THRESHOLDANALYSISCSVWRITER_H
