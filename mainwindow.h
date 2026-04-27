#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "typingsession.h"
#include "sessionsummary.h"

class typingtextedit;


// QT_BEGIN_NAMESPACE
// namespace Ui {
// class MainWindow;
// }
// QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    typingtextedit *typingArea_;
    void startNewSession();
    void handleKeyPressed(int key, qint64 timestampNs, bool autoRepeat);
    void handleKeyReleased(int key, qint64 timestampNs, bool autoRepeat);
    void appendEvent(KeyAction action, int key, qint64 timestampNs, bool autoRepeat);
    TypingSession currentSession_;
    SessionSummary buildSessionSummary() const;
    void updateSessionStatus();
    void fillDwellStats(SessionSummary &summary) const;

};
#endif // MAINWINDOW_H
