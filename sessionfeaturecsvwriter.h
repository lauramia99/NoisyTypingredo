#ifndef SESSIONFEATURECSVWRITER_H
#define SESSIONFEATURECSVWRITER_H

#pragma once

#include "sessionfeaturevector.h"

#include <QString>

namespace SessionFeatureCsvWriter
{
QString defaultFilePath();
bool appendRow(const QString &filePath, const SessionFeatureVector &vector);
}

#endif // SESSIONFEATURECSVWRITER_H
