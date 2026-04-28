#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "typingsession.h"
#include "sessionsummary.h"
#include "sessionfeaturevector.h"

class typingtextedit;
class QComboBox;
class QLineEdit;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void syncSessionMetadataFromUi();
    void startNewSession();
    void handleCapturedKeystroke(const KeystrokeEvent &event);
    void appendEvent(const KeystrokeEvent &event);

    SessionSummary buildSessionSummary() const;
    SessionFeatureVector buildFeatureVector(const SessionSummary &summary) const;
    void fillDwellStats(SessionSummary &summary) const;
    void fillFlightStats(SessionSummary &summary) const;
    void updateSessionStatus();

    void saveCurrentSession();
    void resetCurrentSession();

    typingtextedit *typingArea_ = nullptr;
    QLineEdit *participantIdEdit_ = nullptr;
    QComboBox *samplePurposeCombo_ = nullptr;
    QComboBox *textModeCombo_ = nullptr;
    QLineEdit *promptLabelEdit_ = nullptr;
    QPushButton *saveSessionButton_ = nullptr;
    QPushButton *resetSessionButton_ = nullptr;
    TypingSession currentSession_;
};
#endif // MAINWINDOW_H
