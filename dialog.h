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
    void show_status_resp(const QString&status);
    void vocal_updata_resp(QVariant);

private:
    Ui::Dialog *ui;
    SpiThread   spiDaemon;
    QMutex mutex;
    void vocal_speaker_enable(bool enable);
    void vocal_mic_enable(int id, bool enable);
    void vocal_mic_volume(int id, bool mute, int volume);
    void mic_cfg_write(int id, uint32_t device_id);
    void mic_cfg_read(int id, uint32_t *device_id);
    void mic_updata(MIC_DEVINFO_S mic_info[]);
};

#endif // DIALOG_H
