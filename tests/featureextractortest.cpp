#include "../featureextractor.h"

#include <QTest>

class FeatureExtractorTest : public QObject
{
    Q_OBJECT

private slots:
    void buildsSummaryFromSimplePressRelease();
    void calculatesFlightTimeBetweenPresses();
    void tracksUnmatchedReleaseAndStillPressedKey();
    void includesIgnoredAutoRepeatCountInSummaryAndVector();
};

void FeatureExtractorTest::buildsSummaryFromSimplePressRelease()
{
    TypingSession session;
    session.participantId = "user_01";
    session.samplePurpose = "training";
    session.textMode = "fixed_text";
    session.promptLabel = "test_prompt";

    session.events.append({
        KeyAction::Press,
        Qt::Key_A,
        30,
        65,
        0,
        1000000,
        false
    });

    session.events.append({
        KeyAction::Release,
        Qt::Key_A,
        30,
        65,
        0,
        6000000,
        false
    });

    const SessionSummary summary =
        FeatureExtractor::buildSessionSummary(session);

    QCOMPARE(summary.storedEvents, 2);
    QCOMPARE(summary.pressCount, 1);
    QCOMPARE(summary.releaseCount, 1);
    QCOMPARE(summary.dwellSampleCount, 1);
    QCOMPARE(summary.averageDwellMs, 5.0);

    const SessionFeatureVector vector =
        FeatureExtractor::buildFeatureVector(session, summary);

    QCOMPARE(vector.participantId, QString("user_01"));
    QCOMPARE(vector.samplePurpose, QString("training"));
    QCOMPARE(vector.textMode, QString("fixed_text"));
    QCOMPARE(vector.promptLabel, QString("test_prompt"));
    QCOMPARE(vector.storedEvents, 2);
    QCOMPARE(vector.averageDwellMs, 5.0);
}

void FeatureExtractorTest::calculatesFlightTimeBetweenPresses()
{
    TypingSession session;
    session.participantId = "user_01";
    session.samplePurpose = "training";
    session.textMode = "fixed_text";
    session.promptLabel = "flight_test";

    session.events.append({
        KeyAction::Press,
        Qt::Key_A,
        30,
        65,
        0,
        1000000,
        false
    });

    session.events.append({
        KeyAction::Release,
        Qt::Key_A,
        30,
        65,
        0,
        4000000,
        false
    });

    session.events.append({
        KeyAction::Press,
        Qt::Key_B,
        48,
        66,
        0,
        9000000,
        false
    });

    session.events.append({
        KeyAction::Release,
        Qt::Key_B,
        48,
        66,
        0,
        12000000,
        false
    });

    const SessionSummary summary =
        FeatureExtractor::buildSessionSummary(session);

    QCOMPARE(summary.storedEvents, 4);
    QCOMPARE(summary.pressCount, 2);
    QCOMPARE(summary.releaseCount, 2);
    QCOMPARE(summary.dwellSampleCount, 2);
    QCOMPARE(summary.averageDwellMs, 3.0);
    QCOMPARE(summary.flightSampleCount, 1);
    QCOMPARE(summary.averageFlightMs, 8.0);
}

void FeatureExtractorTest::tracksUnmatchedReleaseAndStillPressedKey()
{
    TypingSession session;
    session.participantId = "user_01";
    session.samplePurpose = "training";
    session.textMode = "free_text";
    session.promptLabel = "broken_event_test";

    session.events.append({
        KeyAction::Release,
        Qt::Key_A,
        30,
        65,
        0,
        1000000,
        false
    });

    session.events.append({
        KeyAction::Press,
        Qt::Key_B,
        48,
        66,
        0,
        3000000,
        false
    });

    const SessionSummary summary =
        FeatureExtractor::buildSessionSummary(session);

    QCOMPARE(summary.storedEvents, 2);
    QCOMPARE(summary.pressCount, 1);
    QCOMPARE(summary.releaseCount, 1);
    QCOMPARE(summary.unmatchedReleaseCount, 1);
    QCOMPARE(summary.keysStillPressedCount, 1);
    QCOMPARE(summary.dwellSampleCount, 0);
    QCOMPARE(summary.averageDwellMs, 0.0);
}

void FeatureExtractorTest::includesIgnoredAutoRepeatCountInSummaryAndVector()
{
    TypingSession session;
    session.participantId = "user_01";
    session.samplePurpose = "training";
    session.textMode = "fixed_text";
    session.promptLabel = "repeat_test";
    session.ignoredAutoRepeatCount = 3;

    session.events.append({
        KeyAction::Press,
        Qt::Key_A,
        30,
        65,
        0,
        1000000,
        false
    });

    session.events.append({
        KeyAction::Release,
        Qt::Key_A,
        30,
        65,
        0,
        6000000,
        false
    });

    const SessionSummary summary =
        FeatureExtractor::buildSessionSummary(session);

    QCOMPARE(summary.storedEvents, 2);
    QCOMPARE(summary.ignoredAutoRepeatCount, 3);

    const SessionFeatureVector vector =
        FeatureExtractor::buildFeatureVector(session, summary);

    QCOMPARE(vector.storedEvents, 2);
    QCOMPARE(vector.ignoredAutoRepeatCount, 3);
    QCOMPARE(vector.ignoredRepeatRatio, 0.6);
}

QTEST_MAIN(FeatureExtractorTest)

#include "featureextractortest.moc"
