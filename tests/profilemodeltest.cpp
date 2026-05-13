#include "../profilemodel.h"

#include <QTest>

class ProfileModelTest : public QObject
{
    Q_OBJECT

private slots:
    void buildsProfileFromTrainingSamples();
    void scoresSampleAgainstUserProfile();
    void verifiesSampleUsingThreshold();
    void scoresSampleWhenStdDevIsZero();
};

void ProfileModelTest::buildsProfileFromTrainingSamples()
{
    QVector<SessionFeatureVector> samples;

    SessionFeatureVector first;
    first.averageDwellMs = 10.0;
    first.averageFlightMs = 20.0;
    samples.append(first);

    SessionFeatureVector second;
    second.averageDwellMs = 20.0;
    second.averageFlightMs = 40.0;
    samples.append(second);

    SessionFeatureVector third;
    third.averageDwellMs = 30.0;
    third.averageFlightMs = 60.0;
    samples.append(third);

    const UserProfile profile =
        ProfileModel::buildProfile("user_01", samples);

    QCOMPARE(profile.participantId, QString("user_01"));
    QCOMPARE(profile.trainingSessionCount, 3);

    QCOMPARE(profile.averageDwellMsMean, 20.0);
    QCOMPARE(profile.averageFlightMsMean, 40.0);

    QCOMPARE(profile.averageDwellMsStdDev, 10.0);
    QCOMPARE(profile.averageFlightMsStdDev, 20.0);
}

void ProfileModelTest::scoresSampleAgainstUserProfile()
{
    UserProfile profile;
    profile.participantId = "user_01";
    profile.trainingSessionCount = 3;
    profile.averageDwellMsMean = 20.0;
    profile.averageDwellMsStdDev = 5.0;
    profile.averageFlightMsMean = 40.0;
    profile.averageFlightMsStdDev = 10.0;

    SessionFeatureVector sample;
    sample.averageDwellMs = 30.0;
    sample.averageFlightMs = 50.0;

    const VerificationScore score =
        ProfileModel::scoreSample(profile, sample);

    QCOMPARE(score.dwellDeviation, 2.0);
    QCOMPARE(score.flightDeviation, 1.0);
    QCOMPARE(score.totalScore, 3.0);
}

void ProfileModelTest::verifiesSampleUsingThreshold()
{
    UserProfile profile;
    profile.averageDwellMsMean = 20.0;
    profile.averageDwellMsStdDev = 5.0;
    profile.averageFlightMsMean = 40.0;
    profile.averageFlightMsStdDev = 10.0;

    SessionFeatureVector sample;
    sample.averageDwellMs = 30.0;
    sample.averageFlightMs = 50.0;

    const VerificationDecision acceptedDecision =
        ProfileModel::verifySample(profile, sample, 3.0);

    QCOMPARE(acceptedDecision.score.totalScore, 3.0);
    QCOMPARE(acceptedDecision.threshold, 3.0);
    QVERIFY(acceptedDecision.accepted);

    const VerificationDecision rejectedDecision =
        ProfileModel::verifySample(profile, sample, 2.5);

    QCOMPARE(rejectedDecision.score.totalScore, 3.0);
    QCOMPARE(rejectedDecision.threshold, 2.5);
    QVERIFY(!rejectedDecision.accepted);
}

void ProfileModelTest::scoresSampleWhenStdDevIsZero()
{
    UserProfile profile;
    profile.averageDwellMsMean = 20.0;
    profile.averageDwellMsStdDev = 0.0;
    profile.averageFlightMsMean = 40.0;
    profile.averageFlightMsStdDev = 0.0;

    SessionFeatureVector sample;
    sample.averageDwellMs = 23.0;
    sample.averageFlightMs = 35.0;

    const VerificationScore score =
        ProfileModel::scoreSample(profile, sample);

    QCOMPARE(score.dwellDeviation, 3.0);
    QCOMPARE(score.flightDeviation, 5.0);
    QCOMPARE(score.totalScore, 8.0);
}

QTEST_MAIN(ProfileModelTest)

#include "profilemodeltest.moc"
