#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "userevent.h"
#include "spithread.h"
#include "mcp2210.h"

class Assert;

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    Assert *pAssert;
    void *DevHandle;

    void closeEvent(QCloseEvent *event);
    bool event(QEvent* event);
    void Post_Event(UserEvent::USER_EVENT_TYPE_E UserType);

private slots:
    void showPrin(const QString&);
    void Vocal_Mic_Sig(int id, int volume);

private:
    Ui::Dialog *ui;
    SpiThread   spiDaemon;
    QMutex mutex;
};

#endif // DIALOG_H