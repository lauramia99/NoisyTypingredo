#include "sessionfeaturecsvwriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QStringList>
#include <QTextStream>

namespace
{
QString projectDataDirectoryPath()
{
    const QString projectRoot = QString::fromUtf8(NOISYTYPING_PROJECT_DIR);
    return QDir(projectRoot).filePath("data");
}

QString formatDouble(double value)
{
    return QLocale::c().toString(value, 'f', 6);
}

QString csvHeader()
{
    return QStringLiteral(
        "participant_id,session_id,started_at_utc,stored_events,press_count,release_count,"
        "ignored_auto_repeat_count,overlap_press_count,unmatched_release_count,"
        "keys_still_pressed_count,duration_ms,average_dwell_ms,min_dwell_ms,max_dwell_ms,"
        "average_flight_ms,min_flight_ms,max_flight_ms,overlap_ratio,"
        "unmatched_release_ratio,ignored_repeat_ratio");
}

QString toCsvRow(const SessionFeatureVector &vector)
{
    QStringList columns;
    columns << vector.participantId;
    columns << vector.sessionId;
    columns << vector.startedAtUtcIso;
    columns << QString::number(vector.storedEvents);
    columns << QString::number(vector.pressCount);
    columns << QString::number(vector.releaseCount);
    columns << QString::number(vector.ignoredAutoRepeatCount);
    columns << QString::number(vector.overlapPressCount);
    columns << QString::number(vector.unmatchedReleaseCount);
    columns << QString::number(vector.keysStillPressedCount);
    columns << formatDouble(vector.durationMs);
    columns << formatDouble(vector.averageDwellMs);
    columns << formatDouble(vector.minDwellMs);
    columns << formatDouble(vector.maxDwellMs);
    columns << formatDouble(vector.averageFlightMs);
    columns << formatDouble(vector.minFlightMs);
    columns << formatDouble(vector.maxFlightMs);
    columns << formatDouble(vector.overlapRatio);
    columns << formatDouble(vector.unmatchedReleaseRatio);
    columns << formatDouble(vector.ignoredRepeatRatio);

    return columns.join(',');
}
}


QString SessionFeatureCsvWriter::defaultFilePath()
{
    return QDir(projectDataDirectoryPath()).filePath("feature_vectors.csv");
}

bool SessionFeatureCsvWriter::appendRow(const QString &filePath,
                                        const SessionFeatureVector &vector)
{
    const QFileInfo fileInfo(filePath);
    QDir parentDir = fileInfo.dir();

    if (!parentDir.exists() && !parentDir.mkpath("."))
    {
        return false;
    }

    const bool needsHeader = !fileInfo.exists() || fileInfo.size() == 0;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);

    if (needsHeader)
    {
        stream << csvHeader() << '\n';
    }

    stream << toCsvRow(vector) << '\n';
    return stream.status() == QTextStream::Ok;
}



// sessionfeaturecsvwriter::sessionfeaturecsvwriter() {}
