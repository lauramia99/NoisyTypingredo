#include "auditeventscsvwriter.h"

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
        "created_at_utc,event_type,participant_id,session_id,details");
}

QString toCsvRow(const AuditEventExportRow &row)
{
    QStringList columns;
    columns << escapeCsv(row.createdAtUtc);
    columns << escapeCsv(row.eventType);
    columns << escapeCsv(row.participantId);
    columns << escapeCsv(row.sessionId);
    columns << escapeCsv(row.details);

    return columns.join(',');
}
}

QString AuditEventsCsvWriter::defaultFilePath()
{
    return QDir(projectDataDirectoryPath()).filePath("audit_events.csv");
}

bool AuditEventsCsvWriter::writeRows(
    const QString &filePath,
    const QVector<AuditEventExportRow> &rows)
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

    for (const AuditEventExportRow &row : rows)
    {
        stream << toCsvRow(row) << '\n';
    }

    return stream.status() == QTextStream::Ok;
}