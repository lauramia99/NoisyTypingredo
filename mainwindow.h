#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>

#include "databasemanager.h"
#include "typingsession.h"

class typingtextedit;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QDoubleSpinBox;
class QLabel;

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
    void showVerificationStats();
    void analyzeThresholds();

    void updateSaveButtonState();
    bool currentSessionIsReadyToSave() const;

    void updateEnrollmentStatus();
    bool ensureEnrollmentComplete(const QString &participantId);

    void populatePromptSelector();
    void updatePromptSelection();
    bool validateCurrentPromptText();

    QString currentParticipantId() const;
    QString promptProgressText() const;
    void refreshParticipantList();

    bool saveCurrentSessionToStorage(bool showSuccessMessage,
                                     bool resetConsentAfterSave);
    void startEnrollment();
    void saveEnrollmentStep();
    void configureEnrollmentStep();
    void updateEnrollmentControls();

    typingtextedit *typingArea_ = nullptr;
    QComboBox *participantIdCombo_ = nullptr;
    QComboBox *samplePurposeCombo_ = nullptr;
    QComboBox *textModeCombo_ = nullptr;
    QComboBox *attemptTypeCombo_ = nullptr;
    QLineEdit *promptLabelEdit_ = nullptr;
    QCheckBox *consentCheckBox_ = nullptr;
    QPushButton *saveSessionButton_ = nullptr;
    QPushButton *resetSessionButton_ = nullptr;
    QPushButton *buildProfileButton_ = nullptr;
    QPushButton *verifySessionButton_ = nullptr;
    QPushButton *verificationStatsButton_ = nullptr;
    QPushButton *thresholdAnalysisButton_ = nullptr;
    QPushButton *startEnrollmentButton_ = nullptr;
    QPushButton *saveAndNextEnrollmentButton_ = nullptr;
    QDoubleSpinBox *verificationThresholdSpinBox_ = nullptr;
    QLabel *enrollmentStatusLabel_ = nullptr;
    QLabel *enrollmentProgressLabel_ = nullptr;
    QComboBox *promptCombo_ = nullptr;

    bool enrollmentActive_ = false;
    int enrollmentStepIndex_ = 0;
    QStringList enrollmentPromptLabels_;

    TypingSession currentSession_;
    DatabaseManager databaseManager_;
};
#endif // MAINWINDOW_H
