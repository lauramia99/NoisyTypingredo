#include "../thresholdanalysis.h"

#include <QTest>

class ThresholdAnalysisTest : public QObject
{
    Q_OBJECT

private slots:
    void calculatesFarAndFrrForCandidateThresholds();
    void selectsBestThresholdWithLowestRateGap();
};

void ThresholdAnalysisTest::calculatesFarAndFrrForCandidateThresholds()
{
    QVector<VerificationResultExportRow> rows;

    VerificationResultExportRow genuine;
    genuine.attemptType = "genuine";
    genuine.totalScore = 2.0;
    rows.append(genuine);

    VerificationResultExportRow impostor;
    impostor.attemptType = "impostor";
    impostor.totalScore = 1.0;
    rows.append(impostor);

    const ThresholdAnalysisResult result =
        ThresholdAnalysis::buildAnalysis(rows, 2.0, 1.0);

    QCOMPARE(result.rows.size(), 3);

    const ThresholdAnalysisRow thresholdOne = result.rows.at(1);
    QCOMPARE(thresholdOne.threshold, 1.0);
    QCOMPARE(thresholdOne.falseRejectCount, 1);
    QCOMPARE(thresholdOne.falseAcceptCount, 1);
    QCOMPARE(thresholdOne.falseRejectRate, 1.0);
    QCOMPARE(thresholdOne.falseAcceptRate, 1.0);

    const ThresholdAnalysisRow thresholdTwo = result.rows.at(2);
    QCOMPARE(thresholdTwo.threshold, 2.0);
    QCOMPARE(thresholdTwo.falseRejectCount, 0);
    QCOMPARE(thresholdTwo.falseAcceptCount, 1);
    QCOMPARE(thresholdTwo.falseRejectRate, 0.0);
    QCOMPARE(thresholdTwo.falseAcceptRate, 1.0);
}

void ThresholdAnalysisTest::selectsBestThresholdWithLowestRateGap()
{
    QVector<VerificationResultExportRow> rows;

    VerificationResultExportRow firstGenuine;
    firstGenuine.attemptType = "genuine";
    firstGenuine.totalScore = 1.0;
    rows.append(firstGenuine);

    VerificationResultExportRow secondGenuine;
    secondGenuine.attemptType = "genuine";
    secondGenuine.totalScore = 2.0;
    rows.append(secondGenuine);

    VerificationResultExportRow firstImpostor;
    firstImpostor.attemptType = "impostor";
    firstImpostor.totalScore = 4.0;
    rows.append(firstImpostor);

    VerificationResultExportRow secondImpostor;
    secondImpostor.attemptType = "impostor";
    secondImpostor.totalScore = 5.0;
    rows.append(secondImpostor);

    const ThresholdAnalysisResult result =
        ThresholdAnalysis::buildAnalysis(rows, 5.0, 1.0);

    QVERIFY(result.hasBestRow);
    QCOMPARE(result.bestRow.threshold, 2.0);
    QCOMPARE(result.bestRow.falseRejectRate, 0.0);
    QCOMPARE(result.bestRow.falseAcceptRate, 0.0);
    QCOMPARE(result.bestRow.balancedErrorRate, 0.0);
}

QTEST_MAIN(ThresholdAnalysisTest)

#include "thresholdanalysistest.moc"
