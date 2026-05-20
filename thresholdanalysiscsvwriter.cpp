#include "thresholdanalysiscsvwriter.h"

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
        "participant_id,threshold,genuine_count,impostor_count,"
        "true_accept,false_reject,false_accept,true_reject,"
        "frr,far,balanced_error_rate,absolute_rate_gap");
}

QString toCsvRow(const QString &participantId,
                 const ThresholdAnalysisRow &row)
{
    QStringList columns;
    columns << escapeCsv(participantId);
    columns << QString::number(row.threshold, 'f', 6);
    columns << QString::number(row.genuineCount);
    columns << QString::number(row.impostorCount);
    columns << QString::number(row.trueAcceptCount);
    columns << QString::number(row.falseRejectCount);
    columns << QString::number(row.falseAcceptCount);
    columns << QString::number(row.trueRejectCount);
    columns << QString::number(row.falseRejectRate, 'f', 6);
    columns << QString::number(row.falseAcceptRate, 'f', 6);
    columns << QString::number(row.balancedErrorRate, 'f', 6);
    columns << QString::number(row.absoluteRateGap, 'f', 6);

    return columns.join(',');
}
}

QString ThresholdAnalysisCsvWriter::defaultFilePath()
{
    return QDir(projectDataDirectoryPath()).filePath("threshold_analysis.csv");
}

bool ThresholdAnalysisCsvWriter::writeRows(
    const QString &filePath,
    const QString &participantId,
    const QVector<ThresholdAnalysisRow> &rows)
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

    for (const ThresholdAnalysisRow &row : rows)
    {
        stream << toCsvRow(participantId, row) << '\n';
    }

    return stream.status() == QTextStream::Ok;
}
