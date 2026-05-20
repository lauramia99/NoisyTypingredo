#include "thresholdanalysis.h"

#include <QtGlobal>

#include <cmath>

namespace
{
bool isKnownAttemptType(const QString &attemptType)
{
    return attemptType == QStringLiteral("genuine") ||
           attemptType == QStringLiteral("impostor");
}

bool shouldReplaceBestRow(const ThresholdAnalysisRow &candidate,
                          const ThresholdAnalysisRow &currentBest)
{
    constexpr double epsilon = 0.000001;

    if (candidate.absoluteRateGap < currentBest.absoluteRateGap - epsilon)
    {
        return true;
    }

    if (std::abs(candidate.absoluteRateGap - currentBest.absoluteRateGap) <= epsilon &&
        candidate.balancedErrorRate < currentBest.balancedErrorRate - epsilon)
    {
        return true;
    }

    if (std::abs(candidate.absoluteRateGap - currentBest.absoluteRateGap) <= epsilon &&
        std::abs(candidate.balancedErrorRate - currentBest.balancedErrorRate) <= epsilon &&
        candidate.threshold < currentBest.threshold)
    {
        return true;
    }

    return false;
}
}

double ThresholdAnalysis::defaultMaxThreshold(
    const QVector<VerificationResultExportRow> &rows)
{
    double maxScore = 0.0;

    for (const VerificationResultExportRow &row : rows)
    {
        if (isKnownAttemptType(row.attemptType) && row.totalScore > maxScore)
        {
            maxScore = row.totalScore;
        }
    }

    const double roundedMax = std::ceil(maxScore * 2.0) / 2.0;
    return qMax(1.0, roundedMax + 0.5);
}

ThresholdAnalysisResult ThresholdAnalysis::buildAnalysis(
    const QVector<VerificationResultExportRow> &rows,
    double maxThreshold,
    double step)
{
    ThresholdAnalysisResult result;

    if (step <= 0.0 || maxThreshold < 0.0)
    {
        return result;
    }

    for (const VerificationResultExportRow &row : rows)
    {
        if (row.attemptType == QStringLiteral("genuine"))
        {
            ++result.genuineCount;
        }
        else if (row.attemptType == QStringLiteral("impostor"))
        {
            ++result.impostorCount;
        }
    }

    const int thresholdCount =
        static_cast<int>(std::floor(maxThreshold / step)) + 1;

    for (int i = 0; i < thresholdCount; ++i)
    {
        ThresholdAnalysisRow analysisRow;
        analysisRow.threshold = i * step;
        analysisRow.genuineCount = result.genuineCount;
        analysisRow.impostorCount = result.impostorCount;

        for (const VerificationResultExportRow &verificationRow : rows)
        {
            const bool acceptedAtThreshold =
                verificationRow.totalScore <= analysisRow.threshold;

            if (verificationRow.attemptType == QStringLiteral("genuine"))
            {
                if (acceptedAtThreshold)
                {
                    ++analysisRow.trueAcceptCount;
                }
                else
                {
                    ++analysisRow.falseRejectCount;
                }
            }
            else if (verificationRow.attemptType == QStringLiteral("impostor"))
            {
                if (acceptedAtThreshold)
                {
                    ++analysisRow.falseAcceptCount;
                }
                else
                {
                    ++analysisRow.trueRejectCount;
                }
            }
        }

        if (analysisRow.genuineCount > 0)
        {
            analysisRow.falseRejectRate =
                static_cast<double>(analysisRow.falseRejectCount) /
                analysisRow.genuineCount;
        }

        if (analysisRow.impostorCount > 0)
        {
            analysisRow.falseAcceptRate =
                static_cast<double>(analysisRow.falseAcceptCount) /
                analysisRow.impostorCount;
        }

        analysisRow.balancedErrorRate =
            (analysisRow.falseRejectRate + analysisRow.falseAcceptRate) / 2.0;
        analysisRow.absoluteRateGap =
            std::abs(analysisRow.falseRejectRate - analysisRow.falseAcceptRate);

        result.rows.append(analysisRow);

        if (analysisRow.genuineCount > 0 && analysisRow.impostorCount > 0)
        {
            if (!result.hasBestRow ||
                shouldReplaceBestRow(analysisRow, result.bestRow))
            {
                result.bestRow = analysisRow;
                result.hasBestRow = true;
            }
        }
    }

    return result;
}

ThresholdAnalysisResult ThresholdAnalysis::buildDefaultAnalysis(
    const QVector<VerificationResultExportRow> &rows)
{
    return buildAnalysis(rows, defaultMaxThreshold(rows), 0.25);
}
