#include "mcp2210.h"
#include "dialog.h"
#include "ui_dialog.h"
#include <QSettings>
#include <QTextCodec>
#include <QCloseEvent>
#include <stdint.h>
#include <QPushButton>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    ui->verticalSlider_mic1->setMinimum(0);
    ui->verticalSlider_mic1->setMaximum(100);
    ui->verticalSlider_mic1->setEnabled(false);
    ui->checkBox_mic1->setEnabled(false);

    ui->verticalSlider_mic2->setMinimum(0);
    ui->verticalSlider_mic2->setMaximum(100);
    ui->verticalSlider_mic2->setEnabled(false);
    ui->checkBox_mic2->setEnabled(false);

    ui->verticalSlider_speaker->setMinimum(0);
    ui->verticalSlider_speaker->setMaximum(100);
    ui->verticalSlider_speaker->setEnabled(false);
    ui->checkBox_speaker->setEnabled(false);

    connect(&spiDaemon, SIGNAL(vocal_connect(int)),
            this, SLOT(vocal_connect_resp(int)));

	connect(&spiDaemon, SIGNAL(vocal_updata(QVariant)),
            this, SLOT(vocal_updata_resp(QVariant)));

    connect(ui->verticalSlider_mic1, SIGNAL(valueChanged(int)),
            &spiDaemon, SLOT(mic_volume_change_resp(int)));
    connect(ui->verticalSlider_mic2, SIGNAL(valueChanged(int)),
            &spiDaemon, SLOT(mic_volume_change_resp(int)));
    connect(ui->checkBox_mic1, SIGNAL(stateChanged(int)),
            &spiDaemon, SLOT(mic_volume_change_resp(int)));
    connect(ui->checkBox_mic2, SIGNAL(stateChanged(int)),
            &spiDaemon, SLOT(mic_volume_change_resp(int)));

    connect(ui->verticalSlider_speaker, SIGNAL(valueChanged(int)),
            &spiDaemon, SLOT(speaker_volume_change_resp(int)));
    connect(ui->checkBox_speaker, SIGNAL(stateChanged(int)),
            &spiDaemon, SLOT(speaker_volume_change_resp(int)));

    connect(ui->pushButton, SIGNAL(clicked(int)),
            &spiDaemon, SLOT(mic_button_pairing_resp()));

    spiDaemon.start();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::closeEvent(QCloseEvent *event)
{
    spiDaemon.stop();
    spiDaemon.wait();
    qDebug("close");
    event->accept();
}

void Dialog::vocal_spk_enable(bool enable)
{
    ui->checkBox_speaker->setEnabled(enable);
    ui->verticalSlider_speaker->setEnabled(enable);
    if(!enable) {
        ui->checkBox_speaker->setChecked(false);
        ui->verticalSlider_speaker->setValue(0);
    }
}


void Dialog::vocal_spk_volume(bool mute, int volume)
{
	ui->checkBox_speaker->setChecked(mute);
    ui->verticalSlider_speaker->setValue(volume);
}


void Dialog::vocal_mic_enable(int id, bool enable, uint32_t device_id)
{
    switch(id) {
    case 0:
        if(!ui->verticalSlider_mic1->isEnabled() && enable) {
			ui->checkBox_mic1->setWhatsThis(QString::number(device_id));
			ui->verticalSlider_mic1->setWhatsThis(QString::number(device_id));
			ui->checkBox_mic1->setEnabled(true);
        	ui->verticalSlider_mic1->setEnabled(true);
		}
		else if (ui->verticalSlider_mic1->isEnabled() && !enable) {
			ui->checkBox_mic1->setEnabled(false);
			ui->checkBox_mic1->setChecked(false);
        	ui->verticalSlider_mic1->setEnabled(false);
            ui->verticalSlider_mic1->setValue(0);
		}
        break;
    case 1:
		if(!ui->verticalSlider_mic2->isEnabled() && enable) {
			ui->checkBox_mic2->setWhatsThis(QString::number(device_id));
			ui->verticalSlider_mic2->setWhatsThis(QString::number(device_id));
			ui->checkBox_mic2->setEnabled(true);
        	ui->verticalSlider_mic2->setEnabled(true);
		}
		else if (ui->verticalSlider_mic2->isEnabled() && !enable) {
			ui->checkBox_mic2->setEnabled(false);
			ui->checkBox_mic2->setChecked(false);
        	ui->verticalSlider_mic2->setEnabled(false);
            ui->verticalSlider_mic2->setValue(0);
		}
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

void Dialog::mic_cfg_write(int id, uint32_t device_id)
{
    QSettings *vocal_cfg = new QSettings("vocal.ini", QSettings::IniFormat);

    vocal_cfg->setValue("/mic/dev" + QString::number(id), QString::number(device_id));

    delete vocal_cfg;
}

void Dialog::mic_cfg_read(int id, uint32_t *device_id)
{
    QSettings *vocal_cfg = new QSettings("vocal.ini", QSettings::IniFormat);

    *device_id = vocal_cfg->value("/mic/dev" + QString::number(id)).toUInt();

    delete vocal_cfg;
}

#define CURVERSION_MIC_MAX_NUM  2

void Dialog::vocal_connect_resp(int enable)
{
    if(enable) {
        ui->label_status->setText("Connect");
    }
    else {
        ui->label_status->setText("Disconnect");
        vocal_spk_enable(false);
        for(int i = 0; i < CURVERSION_MIC_MAX_NUM; i++) {
            vocal_mic_enable(i, false, 0);
        }
    }
	spiDaemon.semaphore.release();
}

void Dialog::mic_updata(MIC_DEVINFO_S mic_info[])
{
    int i, j;
    uint32_t    cfg_mic_devid = 0;
    bool        cfg_mic_free[CURVERSION_MIC_MAX_NUM] = {false};
    bool        mic_process[MIC_MAX_NUM] = {false};

    for(i = 0; i < CURVERSION_MIC_MAX_NUM; i++) {
        mic_cfg_read(i, &cfg_mic_devid);
        if(!cfg_mic_devid) {
            cfg_mic_free[i] = true;
            continue;
        }
        for(j = 0; j < MIC_MAX_NUM; j++) {
            if(cfg_mic_devid == (mic_info[j].device_id)) {
                vocal_mic_enable(i, true, mic_info[j].device_id);
                vocal_mic_volume(i, mic_info[j].mute, mic_info[j].volume);
                mic_process[j] = true;
                break;
            }
        }
        if(j == MIC_MAX_NUM) {
            cfg_mic_free[i] = true;
        }
    }

    for(j = 0; j < MIC_MAX_NUM; j++) {
        if(!mic_info[j].device_id || mic_process[j]) {
            continue;
        }
        for(i = 0; i < CURVERSION_MIC_MAX_NUM; i++) {
            if(cfg_mic_free[i]) {
                vocal_mic_enable(i, true, mic_info[j].device_id);
                vocal_mic_volume(i, mic_info[j].mute, mic_info[j].volume);
                mic_cfg_write(i, mic_info[j].device_id);
                cfg_mic_free[i] = false;
                break;
            }
        }
    }

    for(i = 0; i < CURVERSION_MIC_MAX_NUM; i++) {
        if(cfg_mic_free[i]) {
            vocal_mic_enable(i, false, 0);
        }
    }
}

void Dialog::spk_updata(MIC_DEVINFO_S *spk_dev)
{
    vocal_spk_enable(true);
	vocal_spk_volume(spk_dev->mute, spk_dev->volume);
}

void Dialog::vocal_updata_resp(QVariant dataVar) {
    VOCAL_SYS_STATUS_S  sys = dataVar.value<VOCAL_SYS_STATUS_S>();

    mic_updata(sys.mic_dev);
    spk_updata(&sys.spk_dev);

    spiDaemon.semaphore.release();
}
