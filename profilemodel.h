#ifndef PROFILEMODEL_H
#define PROFILEMODEL_H

#include "sessionfeaturevector.h"

#include <QVector>
#include <QString>

struct UserProfile
{
    QString participantId;
    int trainingSessionCount = 0;

    double averageDwellMsMean = 0.0;
    double averageDwellMsStdDev = 0.0;

    double averageFlightMsMean = 0.0;
    double averageFlightMsStdDev = 0.0;
};

struct VerificationScore
{
    double dwellDeviation = 0.0;
    double flightDeviation = 0.0;
    double totalScore = 0.0;
};

struct VerificationDecision
{
    VerificationScore score;
    double threshold = 0.0;
    bool accepted = false;
};


namespace ProfileModel
{
UserProfile buildProfile(const QString &participantId,
                         const QVector<SessionFeatureVector> &trainingSamples);

VerificationScore scoreSample(const UserProfile &profile,
                              const SessionFeatureVector &sample);

VerificationDecision verifySample(const UserProfile &profile,
                                  const SessionFeatureVector &sample,
                                  double threshold);

}

#endif // PROFILEMODEL_H
