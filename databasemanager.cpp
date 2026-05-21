#include "databasemanager.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "typingsession.h"
#include "sessionsummary.h"

namespace {
const QString kConnectionName = QStringLiteral("noisytyping_connection");

QString actionToString(KeyAction action)
{
    return action == KeyAction::Press ? "press" : "release";
}

QString projectDataDirectoryPath()
{
    const QString projectRoot = QString::fromUtf8(NOISYTYPING_PROJECT_DIR);
    return QDir(projectRoot).filePath("data");
}
}

DatabaseManager::DatabaseManager() = default;

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open()
{
    return open(databaseFilePath());
}

QString DatabaseManager::databaseFilePath() const
{
    return QDir(projectDataDirectoryPath()).filePath("noisytyping.sqlite");
}

bool DatabaseManager::open(const QString &databasePath)
{
    lastErrorText_.clear();

    if (database_.isOpen()) {
        if (database_.databaseName() == databasePath) {
            return true;
        }
        close();
    }

    const QFileInfo fileInfo(databasePath);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        lastErrorText_ = QStringLiteral("Could not create database directory.");
        return false;
    }

    if (QSqlDatabase::contains(kConnectionName)) {
        database_ = QSqlDatabase::database(kConnectionName);
    } else {
        database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConnectionName);
    }

    database_.setDatabaseName(databasePath);

    if (!database_.open()) {
        lastErrorText_ = database_.lastError().text();
        return false;
    }

    return initializeSchema();
}

void DatabaseManager::close()
{
    if (!database_.isValid()) {
        return;
    }

    const QString connectionName = database_.connectionName();

    if (database_.isOpen()) {
        database_.close();
    }

    database_ = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

QString DatabaseManager::lastErrorText() const
{
    return lastErrorText_;
}

bool DatabaseManager::initializeSchema()
{
    if (!database_.isOpen()) {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    QSqlQuery query(database_);

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS participants ("
            "participant_id TEXT PRIMARY KEY,"
            "created_at_utc TEXT NOT NULL,"
            "consent_version TEXT NOT NULL,"
            "is_active INTEGER NOT NULL DEFAULT 1"
            ")"))
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS sessions ("
            "session_id TEXT PRIMARY KEY,"
            "participant_id TEXT NOT NULL,"
            "sample_purpose TEXT NOT NULL,"
            "text_mode TEXT NOT NULL,"
            "prompt_label TEXT,"
            "started_at_utc TEXT NOT NULL,"
            "FOREIGN KEY(participant_id) REFERENCES participants(participant_id)"
            ")"))
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS session_events ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "session_id TEXT NOT NULL,"
            "event_index INTEGER NOT NULL,"
            "action TEXT NOT NULL,"
            "key_code INTEGER NOT NULL,"
            "native_scan_code INTEGER NOT NULL,"
            "native_virtual_key INTEGER NOT NULL,"
            "native_modifiers INTEGER NOT NULL,"
            "timestamp_ns INTEGER NOT NULL,"
            "auto_repeat INTEGER NOT NULL,"
            "FOREIGN KEY(session_id) REFERENCES sessions(session_id)"
            ")"))
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS session_features ("
            "session_id TEXT PRIMARY KEY,"
            "stored_events INTEGER NOT NULL,"
            "press_count INTEGER NOT NULL,"
            "release_count INTEGER NOT NULL,"
            "ignored_auto_repeat_count INTEGER NOT NULL,"
            "overlap_press_count INTEGER NOT NULL,"
            "unmatched_release_count INTEGER NOT NULL,"
            "keys_still_pressed_count INTEGER NOT NULL,"
            "duration_ms REAL NOT NULL,"
            "average_dwell_ms REAL NOT NULL,"
            "min_dwell_ms REAL NOT NULL,"
            "max_dwell_ms REAL NOT NULL,"
            "average_flight_ms REAL NOT NULL,"
            "min_flight_ms REAL NOT NULL,"
            "max_flight_ms REAL NOT NULL,"
            "overlap_ratio REAL NOT NULL,"
            "unmatched_release_ratio REAL NOT NULL,"
            "ignored_repeat_ratio REAL NOT NULL,"
            "FOREIGN KEY(session_id) REFERENCES sessions(session_id)"
            ")"))
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS verification_results ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "session_id TEXT NOT NULL,"
            "participant_id TEXT NOT NULL,"
            "created_at_utc TEXT NOT NULL,"
            "total_score REAL NOT NULL,"
            "dwell_deviation REAL NOT NULL,"
            "flight_deviation REAL NOT NULL,"
            "threshold_value REAL NOT NULL,"
            "accepted INTEGER NOT NULL,"
            "training_session_count INTEGER NOT NULL,"
            "attempt_type TEXT NOT NULL,"
            "FOREIGN KEY(session_id) REFERENCES sessions(session_id),"
            "FOREIGN KEY(participant_id) REFERENCES participants(participant_id)"
            ")"))
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }


    return true;
}

bool DatabaseManager::ensureParticipantExists(const QString &participantId)
{
    if (participantId.trimmed().isEmpty())
    {
        lastErrorText_ = QStringLiteral("Participant ID is empty.");
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(
        "INSERT OR IGNORE INTO participants "
        "(participant_id, created_at_utc, consent_version, is_active) "
        "VALUES (?, ?, ?, ?)");

    query.addBindValue(participantId.trimmed());
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    query.addBindValue(QStringLiteral("dev_v1"));
    query.addBindValue(1);

    if (!query.exec())
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::loadParticipantIds(QStringList &participantIds)
{
    lastErrorText_.clear();
    participantIds.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    QSqlQuery query(database_);

    if (!query.exec(
            "SELECT p.participant_id "
            "FROM participants p "
            "LEFT JOIN sessions s ON s.participant_id = p.participant_id "
            "WHERE p.is_active = 1 "
            "GROUP BY p.participant_id "
            "ORDER BY MAX(s.started_at_utc) DESC, p.participant_id COLLATE NOCASE ASC"))
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    while (query.next())
    {
        participantIds.append(query.value(0).toString());
    }

    return true;
}

bool DatabaseManager::loadEnrollmentStatus(const QString &participantId,
                                           EnrollmentStatus &status)
{
    lastErrorText_.clear();
    status = EnrollmentStatus{};

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    const QString normalizedParticipantId = participantId.trimmed();

    if (normalizedParticipantId.isEmpty())
    {
        lastErrorText_ = QStringLiteral("Participant ID is empty.");
        return false;
    }

    status.prompts = {
        {QStringLiteral("prompt_01"), 3, 0},
        {QStringLiteral("prompt_02"), 3, 0},
        {QStringLiteral("prompt_03"), 3, 0}
    };

    bool isComplete = true;

    for (EnrollmentPromptStatus &promptStatus : status.prompts)
    {
        QSqlQuery query(database_);
        query.prepare(
            "SELECT COUNT(*) "
            "FROM sessions "
            "WHERE participant_id = ? "
            "AND sample_purpose = 'training' "
            "AND text_mode = 'fixed_text' "
            "AND prompt_label = ?");

        query.addBindValue(normalizedParticipantId);
        query.addBindValue(promptStatus.promptLabel);

        if (!query.exec())
        {
            lastErrorText_ = query.lastError().text();
            return false;
        }

        if (!query.next())
        {
            lastErrorText_ = QStringLiteral("Could not read enrollment status.");
            return false;
        }

        promptStatus.completedCount = query.value(0).toInt();

        status.requiredTotal += promptStatus.requiredCount;

        const int cappedCompletedCount =
            promptStatus.completedCount > promptStatus.requiredCount
                ? promptStatus.requiredCount
                : promptStatus.completedCount;

        status.completedTotal += cappedCompletedCount;

        if (promptStatus.completedCount < promptStatus.requiredCount)
        {
            isComplete = false;
        }
    }

    status.isComplete = isComplete;

    return true;
}

bool DatabaseManager::saveSession(const TypingSession &session,
                                  const SessionSummary &summary,
                                  const SessionFeatureVector &features)
{
    Q_UNUSED(summary);

    lastErrorText_.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    const QString sessionId = session.id.toString(QUuid::WithoutBraces);

    if (!database_.transaction())
    {
        lastErrorText_ = database_.lastError().text();
        return false;
    }

    if (!ensureParticipantExists(session.participantId))
    {
        database_.rollback();
        return false;
    }

    QSqlQuery sessionQuery(database_);

    sessionQuery.prepare(
        "INSERT OR REPLACE INTO sessions "
        "(session_id, participant_id, sample_purpose, text_mode, prompt_label, started_at_utc) "
        "VALUES (?, ?, ?, ?, ?, ?)");

    sessionQuery.addBindValue(sessionId);
    sessionQuery.addBindValue(session.participantId);
    sessionQuery.addBindValue(session.samplePurpose);
    sessionQuery.addBindValue(session.textMode);
    sessionQuery.addBindValue(session.promptLabel);
    sessionQuery.addBindValue(session.startedAtUtc.toString(Qt::ISODateWithMs));

    if (!sessionQuery.exec())
    {
        lastErrorText_ = sessionQuery.lastError().text();
        database_.rollback();
        return false;
    }

    QSqlQuery deleteEventsQuery(database_);
    deleteEventsQuery.prepare("DELETE FROM session_events WHERE session_id = ?");
    deleteEventsQuery.addBindValue(sessionId);

    if (!deleteEventsQuery.exec())
    {
        lastErrorText_ = deleteEventsQuery.lastError().text();
        database_.rollback();
        return false;
    }

    QSqlQuery eventQuery(database_);
    eventQuery.prepare(
        "INSERT INTO session_events "
        "(session_id, event_index, action, key_code, native_scan_code, "
        "native_virtual_key, native_modifiers, timestamp_ns, auto_repeat) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

    for (int i = 0; i < session.events.size(); ++i)
    {
        const KeystrokeEvent &event = session.events.at(i);

        eventQuery.bindValue(0, sessionId);
        eventQuery.bindValue(1, i);
        eventQuery.bindValue(2, actionToString(event.action));
        eventQuery.bindValue(3, event.key);
        eventQuery.bindValue(4, event.nativeScanCode);
        eventQuery.bindValue(5, event.nativeVirtualKey);
        eventQuery.bindValue(6, event.nativeModifiers);
        eventQuery.bindValue(7, event.timestampNs);
        eventQuery.bindValue(8, event.autoRepeat ? 1 : 0);

        if (!eventQuery.exec())
        {
            lastErrorText_ = eventQuery.lastError().text();
            database_.rollback();
            return false;
        }
    }

    QSqlQuery featureQuery(database_);
    featureQuery.prepare(
        "INSERT OR REPLACE INTO session_features "
        "(session_id, stored_events, press_count, release_count, "
        "ignored_auto_repeat_count, overlap_press_count, unmatched_release_count, "
        "keys_still_pressed_count, duration_ms, average_dwell_ms, min_dwell_ms, "
        "max_dwell_ms, average_flight_ms, min_flight_ms, max_flight_ms, "
        "overlap_ratio, unmatched_release_ratio, ignored_repeat_ratio) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    featureQuery.addBindValue(sessionId);
    featureQuery.addBindValue(features.storedEvents);
    featureQuery.addBindValue(features.pressCount);
    featureQuery.addBindValue(features.releaseCount);
    featureQuery.addBindValue(features.ignoredAutoRepeatCount);
    featureQuery.addBindValue(features.overlapPressCount);
    featureQuery.addBindValue(features.unmatchedReleaseCount);
    featureQuery.addBindValue(features.keysStillPressedCount);
    featureQuery.addBindValue(features.durationMs);
    featureQuery.addBindValue(features.averageDwellMs);
    featureQuery.addBindValue(features.minDwellMs);
    featureQuery.addBindValue(features.maxDwellMs);
    featureQuery.addBindValue(features.averageFlightMs);
    featureQuery.addBindValue(features.minFlightMs);
    featureQuery.addBindValue(features.maxFlightMs);
    featureQuery.addBindValue(features.overlapRatio);
    featureQuery.addBindValue(features.unmatchedReleaseRatio);
    featureQuery.addBindValue(features.ignoredRepeatRatio);

    if (!featureQuery.exec())
    {
        lastErrorText_ = featureQuery.lastError().text();
        database_.rollback();
        return false;
    }

    if (!database_.commit())
    {
        lastErrorText_ = database_.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::loadSavedSessionCheck(const QString &sessionId,
                                            SavedSessionCheck &check)
{
    lastErrorText_.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    check = SavedSessionCheck{};

    QSqlQuery eventCountQuery(database_);
    eventCountQuery.prepare(
        "SELECT COUNT(*) "
        "FROM session_events "
        "WHERE session_id = ?");

    eventCountQuery.addBindValue(sessionId);

    if (!eventCountQuery.exec())
    {
        lastErrorText_ = eventCountQuery.lastError().text();
        return false;
    }

    if (eventCountQuery.next())
    {
        check.storedEventCount = eventCountQuery.value(0).toInt();
    }
    else
    {
        lastErrorText_ = QStringLiteral("Could not read saved event count.");
        return false;
    }

    QSqlQuery featureQuery(database_);
    featureQuery.prepare(
        "SELECT COUNT(*) "
        "FROM session_features "
        "WHERE session_id = ?");

    featureQuery.addBindValue(sessionId);

    if (!featureQuery.exec())
    {
        lastErrorText_ = featureQuery.lastError().text();
        return false;
    }

    if (featureQuery.next())
    {
        check.hasFeatureRow = featureQuery.value(0).toInt() > 0;
    }
    else
    {
        lastErrorText_ = QStringLiteral("Could not read saved feature count.");
        return false;
    }

    return true;
}

bool DatabaseManager::loadDatabaseStats(DatabaseStats &stats)
{
    lastErrorText_.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    stats = DatabaseStats{};

    QSqlQuery participantQuery(database_);
    if (!participantQuery.exec("SELECT COUNT(*) FROM participants"))
    {
        lastErrorText_ = participantQuery.lastError().text();
        return false;
    }

    if (participantQuery.next())
    {
        stats.participantCount = participantQuery.value(0).toInt();
    }

    QSqlQuery sessionQuery(database_);
    if (!sessionQuery.exec("SELECT COUNT(*) FROM sessions"))
    {
        lastErrorText_ = sessionQuery.lastError().text();
        return false;
    }

    if (sessionQuery.next())
    {
        stats.sessionCount = sessionQuery.value(0).toInt();
    }

    QSqlQuery eventQuery(database_);
    if (!eventQuery.exec("SELECT COUNT(*) FROM session_events"))
    {
        lastErrorText_ = eventQuery.lastError().text();
        return false;
    }

    if (eventQuery.next())
    {
        stats.eventCount = eventQuery.value(0).toInt();
    }

    QSqlQuery featureQuery(database_);
    if (!featureQuery.exec("SELECT COUNT(*) FROM session_features"))
    {
        lastErrorText_ = featureQuery.lastError().text();
        return false;
    }

    if (featureQuery.next())
    {
        stats.featureRowCount = featureQuery.value(0).toInt();
    }

    return true;
}

bool DatabaseManager::loadTrainingFeatureVectors(
    const QString &participantId,
    QVector<SessionFeatureVector> &features)
{
    lastErrorText_.clear();
    features.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    if (participantId.trimmed().isEmpty())
    {
        lastErrorText_ = QStringLiteral("Participant ID is empty.");
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(
        "SELECT "
        "s.participant_id, s.sample_purpose, s.text_mode, s.prompt_label, "
        "s.session_id, s.started_at_utc, "
        "f.stored_events, f.press_count, f.release_count, "
        "f.ignored_auto_repeat_count, f.overlap_press_count, "
        "f.unmatched_release_count, f.keys_still_pressed_count, "
        "f.duration_ms, "
        "f.average_dwell_ms, f.min_dwell_ms, f.max_dwell_ms, "
        "f.average_flight_ms, f.min_flight_ms, f.max_flight_ms, "
        "f.overlap_ratio, f.unmatched_release_ratio, f.ignored_repeat_ratio "
        "FROM session_features f "
        "JOIN sessions s ON s.session_id = f.session_id "
        "WHERE s.participant_id = ? "
        "AND s.sample_purpose = 'training' "
        "AND s.text_mode = 'fixed_text' "
        "AND s.prompt_label IN ('prompt_01', 'prompt_02', 'prompt_03') "
        "ORDER BY s.started_at_utc ASC");

    query.addBindValue(participantId.trimmed());

    if (!query.exec())
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    while (query.next())
    {
        SessionFeatureVector vector;

        vector.participantId = query.value(0).toString();
        vector.samplePurpose = query.value(1).toString();
        vector.textMode = query.value(2).toString();
        vector.promptLabel = query.value(3).toString();
        vector.sessionId = query.value(4).toString();
        vector.startedAtUtcIso = query.value(5).toString();

        vector.storedEvents = query.value(6).toInt();
        vector.pressCount = query.value(7).toInt();
        vector.releaseCount = query.value(8).toInt();
        vector.ignoredAutoRepeatCount = query.value(9).toInt();
        vector.overlapPressCount = query.value(10).toInt();
        vector.unmatchedReleaseCount = query.value(11).toInt();
        vector.keysStillPressedCount = query.value(12).toInt();

        vector.durationMs = query.value(13).toDouble();

        vector.averageDwellMs = query.value(14).toDouble();
        vector.minDwellMs = query.value(15).toDouble();
        vector.maxDwellMs = query.value(16).toDouble();

        vector.averageFlightMs = query.value(17).toDouble();
        vector.minFlightMs = query.value(18).toDouble();
        vector.maxFlightMs = query.value(19).toDouble();

        vector.overlapRatio = query.value(20).toDouble();
        vector.unmatchedReleaseRatio = query.value(21).toDouble();
        vector.ignoredRepeatRatio = query.value(22).toDouble();

        features.append(vector);
    }

    return true;
}

bool DatabaseManager::saveVerificationResult(const VerificationResultRecord &result)
{
    lastErrorText_.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(
        "INSERT INTO verification_results "
        "(session_id, participant_id, attempt_type, created_at_utc, total_score, "
        "dwell_deviation, flight_deviation, threshold_value, accepted, "
        "training_session_count) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(result.sessionId);
    query.addBindValue(result.participantId);
    query.addBindValue(result.attemptType);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    query.addBindValue(result.totalScore);
    query.addBindValue(result.dwellDeviation);
    query.addBindValue(result.flightDeviation);
    query.addBindValue(result.threshold);
    query.addBindValue(result.accepted ? 1 : 0);
    query.addBindValue(result.trainingSessionCount);

    if (!query.exec())
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::loadVerificationStats(const QString &participantId,
                                            VerificationStats &stats)
{
    lastErrorText_.clear();
    stats = VerificationStats{};

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    if (participantId.trimmed().isEmpty())
    {
        lastErrorText_ = QStringLiteral("Participant ID is empty.");
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(
        "SELECT "
        "COUNT(*), "
        "SUM(CASE WHEN accepted = 1 THEN 1 ELSE 0 END), "
        "SUM(CASE WHEN accepted = 0 THEN 1 ELSE 0 END), "
        "AVG(total_score), "
        "MIN(total_score), "
        "MAX(total_score) "
        "FROM verification_results "
        "WHERE participant_id = ?");

    query.addBindValue(participantId.trimmed());

    if (!query.exec())
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    if (!query.next())
    {
        lastErrorText_ = QStringLiteral("Could not read verification statistics.");
        return false;
    }

    stats.totalCount = query.value(0).toInt();
    stats.acceptedCount = query.value(1).toInt();
    stats.rejectedCount = query.value(2).toInt();

    if (stats.totalCount > 0)
    {
        stats.averageTotalScore = query.value(3).toDouble();
        stats.minTotalScore = query.value(4).toDouble();
        stats.maxTotalScore = query.value(5).toDouble();
    }

    QSqlQuery detailQuery(database_);
    detailQuery.prepare(
        "SELECT "
        "SUM(CASE WHEN attempt_type = 'genuine' THEN 1 ELSE 0 END), "
        "SUM(CASE WHEN attempt_type = 'genuine' AND accepted = 1 THEN 1 ELSE 0 END), "
        "SUM(CASE WHEN attempt_type = 'genuine' AND accepted = 0 THEN 1 ELSE 0 END), "
        "SUM(CASE WHEN attempt_type = 'impostor' THEN 1 ELSE 0 END), "
        "SUM(CASE WHEN attempt_type = 'impostor' AND accepted = 1 THEN 1 ELSE 0 END), "
        "SUM(CASE WHEN attempt_type = 'impostor' AND accepted = 0 THEN 1 ELSE 0 END) "
        "FROM verification_results "
        "WHERE participant_id = ?");

    detailQuery.addBindValue(participantId.trimmed());

    if (!detailQuery.exec())
    {
        lastErrorText_ = detailQuery.lastError().text();
        return false;
    }

    if (!detailQuery.next())
    {
        lastErrorText_ = QStringLiteral("Could not read FAR/FRR statistics.");
        return false;
    }

    stats.genuineCount = detailQuery.value(0).toInt();
    stats.genuineAcceptedCount = detailQuery.value(1).toInt();
    stats.genuineRejectedCount = detailQuery.value(2).toInt();

    stats.impostorCount = detailQuery.value(3).toInt();
    stats.impostorAcceptedCount = detailQuery.value(4).toInt();
    stats.impostorRejectedCount = detailQuery.value(5).toInt();

    if (stats.impostorCount > 0)
    {
        stats.falseAcceptRate =
            static_cast<double>(stats.impostorAcceptedCount) / stats.impostorCount;
    }

    if (stats.genuineCount > 0)
    {
        stats.falseRejectRate =
            static_cast<double>(stats.genuineRejectedCount) / stats.genuineCount;
    }

    return true;
}

bool DatabaseManager::loadVerificationResultRows(
    const QString &participantId,
    QVector<VerificationResultExportRow> &rows)
{
    lastErrorText_.clear();
    rows.clear();

    if (!database_.isOpen())
    {
        lastErrorText_ = QStringLiteral("Database is not open.");
        return false;
    }

    if (participantId.trimmed().isEmpty())
    {
        lastErrorText_ = QStringLiteral("Participant ID is empty.");
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(
        "SELECT participant_id, session_id, attempt_type, created_at_utc, "
        "accepted, threshold_value, total_score, dwell_deviation, "
        "flight_deviation, training_session_count "
        "FROM verification_results "
        "WHERE participant_id = ? "
        "ORDER BY created_at_utc ASC");

    query.addBindValue(participantId.trimmed());

    if (!query.exec())
    {
        lastErrorText_ = query.lastError().text();
        return false;
    }

    while (query.next())
    {
        VerificationResultExportRow row;
        row.participantId = query.value(0).toString();
        row.sessionId = query.value(1).toString();
        row.attemptType = query.value(2).toString();
        row.createdAtUtc = query.value(3).toString();
        row.accepted = query.value(4).toInt() != 0;
        row.threshold = query.value(5).toDouble();
        row.totalScore = query.value(6).toDouble();
        row.dwellDeviation = query.value(7).toDouble();
        row.flightDeviation = query.value(8).toDouble();
        row.trainingSessionCount = query.value(9).toInt();

        rows.append(row);
    }

    return true;
}

