#include "spithread.h"
#include <QTime>

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

}

void SpiThread::stop()
{
    stopped = true;
}

void SpiThread::run()
{
    int     count       = 0;
    void    *cur_handle = NULL;
    int     result      = 0;
    VOCAL_SYS_STATUS_S sys_status = {0};
    QTime time;

    while(!stopped) {
        if(!STATUS_CHECK(THR_STA_MCP_FOUND)) {
            if(!MCP2210_GetDevCount(&count)) {
                STATUS_SET(THR_STA_MCP_FOUND);
            }
            else {
                semaphore.acquire();
                emit prin(QString("Disconnect"));
                QThread::currentThread()->msleep(500);
                continue;
            }
        }

        if(!STATUS_CHECK(THR_STA_MCP_OPEN)) {
            if(!MCP2210_Open(0, cur_handle)) {
                STATUS_SET(THR_STA_MCP_OPEN);
                semaphore.acquire();
                emit prin(QString("Connect"));
            }
            else {
                STATUS_CLR(THR_STA_MCP_FOUND);
                QThread::currentThread()->msleep(500);
                continue;
            }
        }

        time.restart();
        result = Vocal_Sys_updata_Process(&sys_status);
        qDebug("%d ms", time.elapsed());

        if(!sys_status.Spi_Conn) {
            if(STATUS_CHECK(THR_STA_MCP_OPEN)) {
                MCP2210_Close(cur_handle);
            }
            STATUS_CLR(THR_STA_MCP_OPEN);
            STATUS_CLR(THR_STA_MCP_FOUND);
        }
        else if(sys_status.Vol_Updata) {
            int i = 0;
            for(i = 0; i < MIC_MAX_NUM; i++) {
                if(sys_status.Mic_Info[i].device_id) {
                    semaphore.acquire();
                    emit vocal_cmd(i+1, sys_status.Mic_Info[i].volume);
                }
            }
            qDebug("%d ms", time.elapsed());
            sys_status.Vol_Updata = 0;
        }

        //QThread::currentThread()->msleep(20);
    }
    if(STATUS_CHECK(THR_STA_MCP_OPEN)) {
        Try_To_close_Net();
        MCP2210_Close(cur_handle);
    }
    stopped = false;
}
