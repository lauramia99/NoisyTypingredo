#ifndef THRESHOLDANALYSIS_H
#define THRESHOLDANALYSIS_H

#include "databasemanager.h"

#include <QString>
#include <QVector>

struct ThresholdAnalysisRow
{
    double threshold = 0.0;

    int genuineCount = 0;
    int impostorCount = 0;

    int trueAcceptCount = 0;
    int falseRejectCount = 0;
    int falseAcceptCount = 0;
    int trueRejectCount = 0;

    double falseRejectRate = 0.0;
    double falseAcceptRate = 0.0;
    double balancedErrorRate = 0.0;
    double absoluteRateGap = 0.0;
};

struct ThresholdAnalysisResult
{
    QVector<ThresholdAnalysisRow> rows;
    ThresholdAnalysisRow bestRow;
    bool hasBestRow = false;

    int genuineCount = 0;
    int impostorCount = 0;
};

namespace ThresholdAnalysis
{
double defaultMaxThreshold(const QVector<VerificationResultExportRow> &rows);

ThresholdAnalysisResult buildAnalysis(
    const QVector<VerificationResultExportRow> &rows,
    double maxThreshold,
    double step);

ThresholdAnalysisResult buildDefaultAnalysis(
    const QVector<VerificationResultExportRow> &rows);
}

#endif // THRESHOLDANALYSIS_H
