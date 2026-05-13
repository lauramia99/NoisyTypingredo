#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "databasemanager.h"
#include "typingsession.h"

class typingtextedit;
class QCheckBox;
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

    void updateSessionStatus();

    void saveCurrentSession();
    void resetCurrentSession();
    void buildCurrentParticipantProfile();
    void verifyCurrentSession();

    typingtextedit *typingArea_ = nullptr;
    QLineEdit *participantIdEdit_ = nullptr;
    QComboBox *samplePurposeCombo_ = nullptr;
    QComboBox *textModeCombo_ = nullptr;
    QLineEdit *promptLabelEdit_ = nullptr;
    QCheckBox *consentCheckBox_ = nullptr;
    QPushButton *saveSessionButton_ = nullptr;
    QPushButton *resetSessionButton_ = nullptr;
    QPushButton *buildProfileButton_ = nullptr;
    QPushButton *verifySessionButton_ = nullptr;

    TypingSession currentSession_;
    DatabaseManager databaseManager_;
};
#endif // MAINWINDOW_H
