#ifndef MCP2210_H
#define MCP2210_H

#ifdef __cplusplus
extern "C"{
#endif
#include <stdint.h>
typedef enum remote_control_cmd_e
{
    OUTPUT_VOLUME_INCREMENT = 2,
    OUTPUT_VOLUME_DECREASE  = 3,
    OUTPUT_VOLUME_MUTE      = 4,
} REMOTE_CONTROL_CMD_E;

typedef struct mic_devinfo_s
{
    uint32_t    device_id;
    uint8_t     ach;
    uint8_t     slot;
    uint8_t     cmd;
    uint8_t     ram_slot;
    uint8_t     mute;
    int16_t     volume;
} MIC_DEVINFO_S;

#define MIC_MAX_NUM 4
typedef struct vocal_sys_status_s
{
    uint8_t         Spi_Conn;
    uint8_t         Net_Enable;
    uint8_t         Net_Updata;
    uint8_t         Vol_Updata;
    uint8_t         Paring_Key;
    MIC_DEVINFO_S   Mic_Info[MIC_MAX_NUM];
} VOCAL_SYS_STATUS_S;

int MCP2210_GetDevCount(int *count);
int MCP2210_Open(int count, void *handle);
int MCP2210_Close(void *handle);

int Vocal_Sys_updata_Process(VOCAL_SYS_STATUS_S *sys_status);
void Try_To_close_Net(void);

#ifdef __cplusplus
}
#endif

#endif // MCP2210_H
