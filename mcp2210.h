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
    uint8_t     ui_cmd;
    uint8_t     ram_slot;
    uint8_t     mute;
    int16_t     volume;
} MIC_DEVINFO_S;

#define MIC_MAX_NUM 4
#define SPK_MAX_NUM 1
typedef struct vocal_sys_status_s
{
    uint8_t         spi_conn;
    uint8_t         nwk_enable;
    uint8_t         nwk_stable;
    uint8_t         sys_updata;
    uint8_t         btn_pairing;
    MIC_DEVINFO_S   mic_dev[MIC_MAX_NUM];
    MIC_DEVINFO_S   spk_dev;
} VOCAL_SYS_STATUS_S;

int     mcp2210_get_dev_count(int *count);
int     mcp2210_open(int count, void *handle);
int     mcp2210_close(void *handle);

int     vocal_working(VOCAL_SYS_STATUS_S *sys_status);
void    vocal_nwk_tryto_close(void);

#ifdef __cplusplus
}
#endif

#endif // MCP2210_H
