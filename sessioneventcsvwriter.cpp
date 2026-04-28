#include "sessioneventcsvwriter.h"

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

QString actionToString(KeyAction action)
{
    return action == KeyAction::Press ? "press" : "release";
}

QString csvHeader()
{
    return QStringLiteral(
        "participant_id,sample_purpose,text_mode,prompt_label,session_id,"
        "started_at_utc,event_index,action,key,native_scan_code,"
        "native_virtual_key,native_modifiers,timestamp_ns,auto_repeat");
}

QString toCsvRow(const TypingSession &session, int eventIndex, const KeystrokeEvent &event)
{
    QStringList columns;
    columns << escapeCsv(session.participantId);
    columns << escapeCsv(session.samplePurpose);
    columns << escapeCsv(session.textMode);
    columns << escapeCsv(session.promptLabel);
    columns << escapeCsv(session.id.toString(QUuid::WithoutBraces));
    columns << escapeCsv(session.startedAtUtc.toString(Qt::ISODateWithMs));
    columns << QString::number(eventIndex);
    columns << actionToString(event.action);
    columns << QString::number(event.key);
    columns << QString::number(event.nativeScanCode);
    columns << QString::number(event.nativeVirtualKey);
    columns << QString::number(event.nativeModifiers);
    columns << QString::number(event.timestampNs);
    columns << (event.autoRepeat ? "1" : "0");

    return columns.join(',');
}
}

QString SessionEventCsvWriter::defaultDirectoryPath()
{
    return QDir(projectDataDirectoryPath()).filePath("raw_sessions");
}

QString SessionEventCsvWriter::sessionFilePath(const TypingSession &session)
{
    const QString fileName = session.id.toString(QUuid::WithoutBraces) + ".csv";
    return QDir(defaultDirectoryPath()).filePath(fileName);
}

bool SessionEventCsvWriter::writeSession(const QString &filePath,
                                         const TypingSession &session)
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

    for (int i = 0; i < session.events.size(); ++i)
    {
        stream << toCsvRow(session, i, session.events.at(i)) << '\n';
    }

    return stream.status() == QTextStream::Ok;
}






