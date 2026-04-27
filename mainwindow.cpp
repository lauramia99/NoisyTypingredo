#include "mainwindow.h"
#include "typingtextedit.h"

#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>
#include <QDateTime>
#include <QUuid>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("NoisyTyping");
    resize(900, 600);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);

    typingArea_ = new typingtextedit(centralWidget);
    typingArea_->setPlaceholderText("Start typing...");

    layout->addWidget(typingArea_);
    setCentralWidget(centralWidget);


    connect(typingArea_, &typingtextedit::keyPressed, this,
            &MainWindow::handleKeyPressed);

    connect(typingArea_, &typingtextedit::keyReleased, this,
            &MainWindow::handleKeyReleased);

    startNewSession();




}

void MainWindow::startNewSession()
{
    currentSession_.id = QUuid::createUuid();
    currentSession_.startedAtUtc = QDateTime::currentDateTimeUtc();
    currentSession_.events.clear();
}

void MainWindow::appendEvent(KeyAction action, int key, qint64 timestampNs, bool autoRepeat)
{
    KeystrokeEvent event{action, key, timestampNs, autoRepeat};
    currentSession_.events.append(event);
}

void MainWindow::handleKeyPressed(int key, qint64 timestampNs, bool autoRepeat)
{
    appendEvent(KeyAction::Press, key, timestampNs, autoRepeat);
    qDebug() << "Session events:" << currentSession_.events.size();
}

void MainWindow::handleKeyReleased(int key, qint64 timestampNs, bool autoRepeat)
{
    appendEvent(KeyAction::Release, key, timestampNs, autoRepeat);
    qDebug() << "Session events:" << currentSession_.events.size();
}




