#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

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

private slots:
    void show_status_resp(const QString&status);
    void vocal_updata_resp(QVariant);

private:
    Ui::Dialog *ui;
    SpiThread   spiDaemon;

    void vocal_spk_enable(bool enable);
    void vocal_spk_volume(bool mute, int volume);
    void vocal_mic_enable(int id, bool enable, uint32_t device_id);
    void vocal_mic_volume(int id, bool mute, int volume);
    void mic_cfg_write(int id, uint32_t device_id);
    void mic_cfg_read(int id, uint32_t *device_id);
    void spk_updata(MIC_DEVINFO_S *spk_dev);
    void mic_updata(MIC_DEVINFO_S mic_info[]);

};

#endif // DIALOG_H
