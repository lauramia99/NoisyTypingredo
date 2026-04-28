#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "typingsession.h"
#include "sessionsummary.h"
#include "sessionfeaturevector.h"


class typingtextedit;
class QPushButton;
class QLineEdit;


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
    void handleCapturedKeystroke(const KeystrokeEvent &event);
    void appendEvent(const KeystrokeEvent &event);
    TypingSession currentSession_;
    SessionSummary buildSessionSummary() const;
    void updateSessionStatus();
    void fillDwellStats(SessionSummary &summary) const;
    void fillFlightStats(SessionSummary &summary) const;

    SessionFeatureVector buildFeatureVector(const SessionSummary &summary) const;

    void saveCurrentSession();
    void resetCurrentSession();

    QPushButton *saveSessionButton_;
    QPushButton *resetSessionButton_;

    QLineEdit *participantIdEdit_;

};
#endif // MAINWINDOW_H
