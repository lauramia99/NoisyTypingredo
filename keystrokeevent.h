#ifndef KEYSTROKEEVENT_H
#define KEYSTROKEEVENT_H

#pragma once

#include <QtTypes>

enum class KeyAction
{
    Press,
    Release
};

struct KeystrokeEvent
{
    KeyAction action;
    int key;
    quint32 nativeScanCode;
    quint32 nativeVirtualKey;
    quint32 nativeModifiers;
    qint64 timestampNs;
    bool autoRepeat;
};

#endif // KEYSTROKEEVENT_H
