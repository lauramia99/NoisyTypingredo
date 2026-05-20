#include "verificationresultscsvwriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

namespace
{
QString projectDataDirectoryPath()
{
    const QString projectRoot = QString::fromUtf8(NOISYTYPING_PROJECT_DIR);
    return QDir(projectRoot).filePath("data");
}

QString escapeCsv(QString value)
{
    value.replace('"', "\"\"");

    if (value.contains(',') || value.contains('"') ||
        value.contains('\n') || value.contains('\r'))
    {
        return '"' + value + '"';
    }

    return value;
}

QString csvHeader()
{
    return QStringLiteral(
        "participant_id,session_id,attempt_type,created_at_utc,accepted,"
        "threshold,total_score,dwell_deviation,flight_deviation,"
        "training_session_count");
}

QString toCsvRow(const VerificationResultExportRow &row)
{
    QStringList columns;
    columns << escapeCsv(row.participantId);
    columns << escapeCsv(row.sessionId);
    columns << escapeCsv(row.attemptType);
    columns << escapeCsv(row.createdAtUtc);
    columns << (row.accepted ? "1" : "0");
    columns << QString::number(row.threshold, 'f', 6);
    columns << QString::number(row.totalScore, 'f', 6);
    columns << QString::number(row.dwellDeviation, 'f', 6);
    columns << QString::number(row.flightDeviation, 'f', 6);
    columns << QString::number(row.trainingSessionCount);

    return columns.join(',');
}
}

QString VerificationResultsCsvWriter::defaultFilePath()
{
    return QDir(projectDataDirectoryPath()).filePath("verification_results.csv");
}

bool VerificationResultsCsvWriter::writeRows(
    const QString &filePath,
    const QVector<VerificationResultExportRow> &rows)
{
    const QFileInfo fileInfo(filePath);
    QDir parentDir = fileInfo.dir();

    if (!parentDir.exists() && !parentDir.mkpath("."))
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);
    stream << csvHeader() << '\n';

    for (const VerificationResultExportRow &row : rows)
    {
        stream << toCsvRow(row) << '\n';
    }

    return stream.status() == QTextStream::Ok;
}
