#ifndef TYPINGTEXTEDIT_H
#define TYPINGTEXTEDIT_H

#include "keystrokeevent.h"

#include <QElapsedTimer>
#include <QPlainTextEdit>
#include <QString>

class QKeyEvent;
class QMimeData;

class typingtextedit final : public QPlainTextEdit
{
    Q_OBJECT

public:
    int typedCharacterCount() const;
    int promptCharacterCount() const;
    int mistakeCount() const;

    explicit typingtextedit(QWidget *parent = nullptr);

    void setPromptText(const QString &promptText);
    QString promptText() const;
    QString typedText() const;
    bool isCompleteAndCorrect() const;
    void resetTypedText();

signals:
    void typingProgressChanged();
    void keystrokeCaptured(const KeystrokeEvent &event);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    void refreshPromptDisplay();

    QElapsedTimer timer_;
    QString promptText_;
    QString typedText_;
};

#endif // TYPINGTEXTEDIT_H
