#include "mcp2210.h"
#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    connect(&spiDaemon, SIGNAL(prin(const QString&)),
            this, SLOT(showPrin(const QString&)));
    connect(&spiDaemon, SIGNAL(vocal_cmd(int, int)),
            this, SLOT(Vocal_Mic_Sig(int, int)));
    spiDaemon.start();

    ui->verticalSlider_mic1->setMinimum(0);
    ui->verticalSlider_mic1->setMaximum(100);

    ui->verticalSlider_mic2->setMinimum(0);
    ui->verticalSlider_mic2->setMaximum(100);
    //accept();
    //pAssert->Ret(E_ERR_UNKOWN_ERROR);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::closeEvent(QCloseEvent *event)
{
    spiDaemon.stop();
    spiDaemon.wait();
    event->accept();
}

bool Dialog::event(QEvent* event)
{
    if(event->type() == UserEvent::GetType()) {
        UserEvent *pUserEvent = (UserEvent *)event;
        switch(pUserEvent->e_UserType) {
        case UserEvent::USER_EVENT_ASSERT_ERR:
            qDebug("close()");
            close();
            break;
        case UserEvent::USER_EVENT_NOTHING:
        default:
            break;
        }
    }
    else {
        return QDialog::event(event);
    }

    return false;
}

void Dialog::Post_Event(UserEvent::USER_EVENT_TYPE_E UserType)
{
    UserEvent *pEvent = new UserEvent;
    pEvent->e_UserType = UserType;
    QCoreApplication::instance()->postEvent(this, pEvent);
}

void Dialog::showPrin(const QString &thread_id)
{
    ui->label->setText(thread_id);
    spiDaemon.semaphore.release();
}

#if 1
void Dialog::Vocal_Mic_Sig(int id, int volume)
{
    switch(id) {
    case 1:
        ui->verticalSlider_mic1->setValue(volume);
        break;
    case 2:
        ui->verticalSlider_mic2->setValue(volume);
        break;
    default:
        break;
    }
    spiDaemon.semaphore.release();
}
#endif
