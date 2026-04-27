#ifndef TYPINGTEXTEDIT_H
#define TYPINGTEXTEDIT_H

#pragma once

#include "keystrokeevent.h"

#include <QElapsedTimer>
#include <QPlainTextEdit>

class QKeyEvent;


class typingtextedit final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit typingtextedit(QWidget *parent = nullptr);

signals:
    // void keyPressed(int key, qint64 timestampNs, bool autoRepeat);
    // void keyReleased(int key, qint64 timestampNs, bool autoRepeat);
    void keystrokeCaptured(const KeystrokeEvent &event);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    QElapsedTimer timer_;

};

#endif // TYPINGTEXTEDIT_H
