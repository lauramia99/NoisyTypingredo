#ifndef SESSIONEVENTCSVWRITER_H
#define SESSIONEVENTCSVWRITER_H

#pragma once

#include "typingsession.h"

#include <QString>

namespace SessionEventCsvWriter
{
QString defaultDirectoryPath();
QString sessionFilePath(const TypingSession &session);
bool writeSession(const QString &filePath, const TypingSession &session);
}

#endif // SESSIONEVENTCSVWRITER_H
