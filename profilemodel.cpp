#include "profilemodel.h"

#include <QtMath>

namespace
{
double mean(const QVector<double> &values)
{
    if (values.isEmpty())
    {
        return 0.0;
    }

    double total = 0.0;

    for (double value : values)
    {
        total += value;
    }

    return total / values.size();
}

double sampleStdDev(const QVector<double> &values, double valueMean)
{
    if (values.size() < 2)
    {
        return 0.0;
    }

    double squaredDiffTotal = 0.0;

    for (double value : values)
    {
        const double diff = value - valueMean;
        squaredDiffTotal += diff * diff;
    }

    return qSqrt(squaredDiffTotal / (values.size() - 1));
}

double normalizedDeviation(double value, double meanValue, double stdDev)
{
    const double absoluteDeviation = qAbs(value - meanValue);

    if (stdDev <= 0.0)
    {
        return absoluteDeviation;
    }

    return absoluteDeviation / stdDev;
}

}

UserProfile ProfileModel::buildProfile(
    const QString &participantId,
    const QVector<SessionFeatureVector> &trainingSamples)
{
    UserProfile profile;
    profile.participantId = participantId;
    profile.trainingSessionCount = trainingSamples.size();
    profile.modelVersion = QStringLiteral("baseline_v1");
    profile.featureSetVersion = QStringLiteral("timing_basic_v1");

    QVector<double> dwellValues;
    QVector<double> flightValues;

    for (const SessionFeatureVector &sample : trainingSamples)
    {
        dwellValues.append(sample.averageDwellMs);
        flightValues.append(sample.averageFlightMs);
    }

    profile.averageDwellMsMean = mean(dwellValues);
    profile.averageFlightMsMean = mean(flightValues);

    profile.averageDwellMsStdDev =
        sampleStdDev(dwellValues, profile.averageDwellMsMean);
    profile.averageFlightMsStdDev =
        sampleStdDev(flightValues, profile.averageFlightMsMean);

    return profile;
}

VerificationScore ProfileModel::scoreSample(const UserProfile &profile,
                                            const SessionFeatureVector &sample)
{
    VerificationScore score;

    score.dwellDeviation = normalizedDeviation(
        sample.averageDwellMs,
        profile.averageDwellMsMean,
        profile.averageDwellMsStdDev);

    score.flightDeviation = normalizedDeviation(
        sample.averageFlightMs,
        profile.averageFlightMsMean,
        profile.averageFlightMsStdDev);

    score.totalScore = score.dwellDeviation + score.flightDeviation;

    return score;
}

VerificationDecision ProfileModel::verifySample(const UserProfile &profile,
                                                const SessionFeatureVector &sample,
                                                double threshold)
{
    VerificationDecision decision;
    decision.score = scoreSample(profile, sample);
    decision.threshold = threshold;
    decision.accepted = decision.score.totalScore <= threshold;

    return decision;
}
