#include "spithread.h"
#include <QTime>
#include <QtWidgets/QSlider>
#include <QtWidgets/QCheckBox>

#define BIT_ISSET(a, s)     (((a) >> (s)) & 0x1)
#define BIT_SET(a, s)       ((a) = (a) | 0x1 << (s))
#define BIT_CLR(a, s)       ((a) = (a) & ~(0x1 << (s)))

#define STATUS_CHECK(s)     BIT_ISSET(status, s)
#define STATUS_SET(s)       BIT_SET(status, s)
#define STATUS_CLR(s)       BIT_CLR(status, s)

typedef enum thread_status_type_e{
    THR_STA_MCP_FOUND = 0,
    THR_STA_MCP_OPEN,
    THR_STA_SPI_TEST,
} THREAD_STATUS_TYPE_E;

SpiThread::SpiThread() : semaphore(1)
{
    stopped = false;
    status  = 0;
}

SpiThread::~SpiThread()
{
    //QVariant DataVar;
    //DataVar.setValue(sys_status);
    //qRegisterMetaType<QVariant>("QVariant");
}

void SpiThread::stop()
{
    stopped = true;
}

void SpiThread::mic_volume_change_resp(int data)
{
    QSlider     *slider = NULL;
    QCheckBox   *checkbox = NULL;
    uint32_t    device_id = 0;

    if(slider = dynamic_cast<QSlider*>(sender())) {
        device_id = slider->whatsThis().toUInt();
    }
    else if(checkbox = dynamic_cast<QCheckBox*>(sender())) {
        device_id = checkbox->whatsThis().toUInt();
    }

    for(int i = 0; i < MIC_MAX_NUM; i++) {
        if(device_id && sys_status.mic_dev[i].device_id == device_id) {
            if(slider) {
                sys_status.mic_dev[i].volume = data;
                //qDebug("mic[%d]:id=0x%08x, volume=%d", i, device_id, data);
            }
            else if(checkbox) {
                sys_status.mic_dev[i].mute   = data ? true : false;
                //qDebug("mic[%d]:id=0x%08x, mute=%d", i, device_id, data);
            }
            sys_status.mic_dev[i].ui_cmd = true;
        }
    }
}

void SpiThread::speaker_volume_change_resp(int data)
{
    QSlider     *slider = NULL;
    QCheckBox   *checkbox = NULL;

    if(slider = dynamic_cast<QSlider*>(sender())) {
        sys_status.spk_dev.volume = data;
        //qDebug("speaker, volume=%d", data);
    }
    else if(checkbox = dynamic_cast<QCheckBox*>(sender())) {
        //qDebug("speaker, mute=%d", data);
        sys_status.spk_dev.mute = data ? true : false;;
    }
    sys_status.spk_dev.ui_cmd = true;
}

void SpiThread::run()
{
    int     count       = 0;
    void    *cur_handle = NULL;
    int     result      = 0;

    QTime       time;
    QVariant    sigarg;
    static int times = 0;
    memset(&sys_status, 0, sizeof(sys_status));

    while(!stopped) {
        if(!STATUS_CHECK(THR_STA_MCP_FOUND)) {
            if(!mcp2210_get_dev_count(&count)) {
                STATUS_SET(THR_STA_MCP_FOUND);
            }
            else {
                semaphore.acquire();
                emit show_status(QString("Disconnect"));
                QThread::currentThread()->msleep(500);
                continue;
            }
        }

        if(!STATUS_CHECK(THR_STA_MCP_OPEN)) {
            if(!mcp2210_open(0, cur_handle)) {
                STATUS_SET(THR_STA_MCP_OPEN);
                semaphore.acquire();
                emit show_status(QString("Connect"));
            }
            else {
                STATUS_CLR(THR_STA_MCP_FOUND);
                QThread::currentThread()->msleep(500);
                continue;
            }
        }

        time.restart();
        result = vocal_working(&sys_status);
        //qDebug("%d ms, %d", time.elapsed(), times++);

        if(!sys_status.spi_conn) {
            if(STATUS_CHECK(THR_STA_MCP_OPEN)) {
                mcp2210_close(cur_handle);
            }
            STATUS_CLR(THR_STA_MCP_OPEN);
            STATUS_CLR(THR_STA_MCP_FOUND);
        }
        else if(sys_status.sys_updata) {
            sigarg.setValue(sys_status);

            semaphore.acquire();
            emit vocal_updata(sigarg);

            qDebug("%d ms", time.elapsed());
            sys_status.sys_updata = 0;
        }
    }
    if(STATUS_CHECK(THR_STA_MCP_OPEN)) {
        vocal_nwk_tryto_close();
        mcp2210_close(cur_handle);
    }
    stopped = false;
}
