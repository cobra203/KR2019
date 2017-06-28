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

    connect(&spiDaemon, SIGNAL(vocal_updata(QVariant)),
            this, SLOT(vocal_updata_resp(QVariant)));

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
    ui->label_status->setText(thread_id);
    spiDaemon.semaphore.release();
}

void Dialog::vocal_mic_enable(int id, bool enable)
{
    switch(id) {
    case 0:
        ui->checkBox_mic1->setEnabled(enable);
        ui->verticalSlider_mic1->setEnabled(enable);
        break;
    case 1:
        ui->checkBox_mic2->setEnabled(enable);
        ui->verticalSlider_mic2->setEnabled(enable);
        break;
    default:
        break;
    }
}

void Dialog::vocal_mic_volume(int id, bool mute, int volume)
{
    switch(id) {
    case 0:
        ui->checkBox_mic1->setChecked(mute);
        ui->verticalSlider_mic1->setValue(volume);
        break;
    case 1:
        ui->checkBox_mic2->setChecked(mute);
        ui->verticalSlider_mic2->setValue(volume);
        break;
    default:
        break;
    }
}

void Dialog::vocal_updata_resp(QVariant dataVar) {
    VOCAL_SYS_STATUS_S sys;

    sys = dataVar.value<VOCAL_SYS_STATUS_S>();

    int i;
    for(i = 0; i < MIC_MAX_NUM; i++) {
        if(sys.Mic_Info[i].device_id) {
            vocal_mic_enable(i, true);
            vocal_mic_volume(i, sys.Mic_Info[i].mute, sys.Mic_Info[i].volume);
        }
        else {
            vocal_mic_enable(i, false);
        }
    }
    spiDaemon.semaphore.release();
}
