#include "verificationstatscsvwriter.h"

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
        "participant_id,total_count,accepted_count,rejected_count,"
        "genuine_count,genuine_accepted_count,genuine_rejected_count,false_reject_rate,"
        "impostor_count,impostor_accepted_count,impostor_rejected_count,false_accept_rate,"
        "average_total_score,min_total_score,max_total_score");
}

QString toCsvRow(const QString &participantId, const VerificationStats &stats)
{
    QStringList columns;
    columns << escapeCsv(participantId);
    columns << QString::number(stats.totalCount);
    columns << QString::number(stats.acceptedCount);
    columns << QString::number(stats.rejectedCount);

    columns << QString::number(stats.genuineCount);
    columns << QString::number(stats.genuineAcceptedCount);
    columns << QString::number(stats.genuineRejectedCount);
    columns << QString::number(stats.falseRejectRate, 'f', 6);

    columns << QString::number(stats.impostorCount);
    columns << QString::number(stats.impostorAcceptedCount);
    columns << QString::number(stats.impostorRejectedCount);
    columns << QString::number(stats.falseAcceptRate, 'f', 6);

    columns << QString::number(stats.averageTotalScore, 'f', 6);
    columns << QString::number(stats.minTotalScore, 'f', 6);
    columns << QString::number(stats.maxTotalScore, 'f', 6);

    return columns.join(',');
}
}

QString VerificationStatsCsvWriter::defaultFilePath()
{
    return QDir(projectDataDirectoryPath()).filePath("verification_stats.csv");
}

bool VerificationStatsCsvWriter::appendRow(const QString &filePath,
                                           const QString &participantId,
                                           const VerificationStats &stats)
{
    const QFileInfo fileInfo(filePath);
    QDir parentDir = fileInfo.dir();

    if (!parentDir.exists() && !parentDir.mkpath("."))
    {
        return false;
    }

    const bool shouldWriteHeader = !fileInfo.exists() || fileInfo.size() == 0;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return false;
    }

    QTextStream stream(&file);

    if (shouldWriteHeader)
    {
        stream << csvHeader() << '\n';
    }

    stream << toCsvRow(participantId, stats) << '\n';

    return stream.status() == QTextStream::Ok;
}
