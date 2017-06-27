#include <QDebug>
#include <QString>
#include <QThread>

#define DEBUG_ENABLE

#if defined(DEBUG_ENABLE)
    #define myDebug(fmt, ...) qDebug(fmt, ##__VA_ARGS__)

void DataDump(const char title[], const uchar data[], int len)
{
    int     i = 0;
    QString Str;

    uchar   *pdata = (uchar *)data;

    Str.sprintf("%s : {", title);
    for(i = 0; i < len; i++) {
        QString nStr;
        Str += nStr.sprintf("%02x", pdata[i]);
        if(i != len - 1) {
            Str += QString(",");
        }
    }
    Str += QString("}");
    myDebug(Str.toLatin1().data());
}

#else
    #define myDebug(fmt, ...)
#endif



void sleep(int ms)
{
    QThread::currentThread()->msleep(ms);
}

#ifdef __cplusplus
extern "C"{
#endif

#define MCP2210_LIB
#include <mcp2210_dll_um.h>
#pragma comment(lib, "legacy_stdio_definitions.lib")

#include "mcp2210.h"
#include <stdint.h>

/**********************************************************************************/
/* MCP2210 Init Functions                                                         */
/**********************************************************************************/
#define DEFAULT_VID     0x4d8
#define DEFAULT_PID     0xde

typedef enum gpio_pin_e
{
    GPIO_0  = 0,
    GPIO_1,
    GPIO_2,
    GPIO_3,
    GPIO_4,
    GPIO_5,
    GPIO_6,
    GPIO_7,
    GPIO_8,
} GPIO_PIN_E;

typedef struct gpio_config_s
{
    uint8_t     CfgSelector;
    uint8_t     GpioPinDes[MCP2210_GPIO_NR];
    uint32_t    DfltGpioOutput;
    uint32_t    DfltGpioDir;
    uint8_t     PwtWakeUpEn;
    uint8_t     IntPinMode;
    uint8_t     SpiBusRelEn;
} GPIO_CONFIG_S;

typedef struct spi_config_s
{
    uint8_t     CfgSelector;
    uint32_t    BaudRate;
    uint32_t    IdleCsVal;
    uint32_t    ActiveCsVal;
    uint32_t    CsToDataDly;
    uint32_t    DataToCsDly;
    uint32_t    DataToDataDly;
    uint32_t    TxferSize;
    uint8_t     SpiMode;
} SPI_CONFIG_S;

typedef struct spi_status_s
{
    uint8_t     SpiExtReqState;
    uint8_t     SpiOwner;
    uint8_t     SpiTxferState;
} SPI_STAUTS_S;

void *cur_handle;

static int _GpioInit(void *handle)
{
    int ret = 0;
    GPIO_CONFIG_S cfg = {0};

    cfg.CfgSelector         = MCP2210_VM_CONFIG;
    cfg.GpioPinDes[GPIO_5]  = MCP2210_PIN_DES_CS;
    cfg.GpioPinDes[GPIO_3]  = MCP2210_PIN_DES_FN;//LED
    cfg.DfltGpioOutput      = 0x20;
    cfg.DfltGpioDir         = 0x0;
    cfg.PwtWakeUpEn         = MCP2210_REMOTE_WAKEUP_ENABLED;
    cfg.IntPinMode          = MCP2210_INT_MD_CNT_NONE;
    cfg.SpiBusRelEn         = MCP2210_SPI_BUS_RELEASE_DISABLED;

    ret = Mcp2210_SetGpioConfig(handle, cfg.CfgSelector, (uint8_t *)&cfg.GpioPinDes, cfg.DfltGpioOutput, cfg.DfltGpioDir,
                          cfg.PwtWakeUpEn, cfg.IntPinMode, cfg.SpiBusRelEn);
    myDebug("GpioInit_ret=%d", ret);

    return ret;
}

static int _SpiInit(void *handle)
{
    int ret = 0;
    SPI_CONFIG_S cfg = {0};

    cfg.CfgSelector         = MCP2210_VM_CONFIG;
    cfg.BaudRate            = 1000000;
    cfg.IdleCsVal           = 0x20;
    cfg.ActiveCsVal         = 0x0;
    cfg.CsToDataDly         = 0;
    cfg.DataToCsDly         = 0;
    cfg.DataToDataDly       = 0;
    cfg.TxferSize           = 2;
    cfg.SpiMode             = MCP2210_SPI_MODE0;

    ret = Mcp2210_SetSpiConfig(handle, cfg.CfgSelector, &cfg.BaudRate, &cfg.IdleCsVal, &cfg.ActiveCsVal,
                                &cfg.CsToDataDly, &cfg.DataToCsDly, &cfg.DataToDataDly,
                                &cfg.TxferSize, &cfg.SpiMode);
    myDebug("SpiInit_ret=%d", ret);

    return ret;
}

int MCP2210_GetDevCount(int *count)
{
    *count = Mcp2210_GetConnectedDevCount(DEFAULT_VID, DEFAULT_PID);

    if(*count <= 0) {
        return E_ERR_DEVICE_NOT_FOUND;
    }

    return Mcp2210_GetLastError();
}

int MCP2210_Open(int count, void *handle)
{
    wchar_t buff[128] = {0};
    unsigned long size;

    handle = Mcp2210_OpenByIndex(DEFAULT_VID, DEFAULT_PID, count, buff, &size);

    if((int)handle <= 0) {
        return Mcp2210_GetLastError();
    }

    if(_GpioInit(handle) || _SpiInit(handle)) {
        return Mcp2210_GetLastError();
    }

    cur_handle = handle;

    return E_SUCCESS;
}

int MCP2210_Close(void *handle)
{
    return Mcp2210_Close(handle);
}

#define SIT_DEVICE_ID 0x0
#define SIT_VOLUME    (sizeof(uint32_t) * MIC_MAX_NUM)
#define RECORD_GET    0
#define RECORD_SET    1

static int _MCP2210_Record(uint8_t ops, uint8_t type_sit, uint8_t slot, void *data)
{
    int     i       = 0;
    uint8_t size    = type_sit ? sizeof(uint16_t) : sizeof(uint32_t);
    uint8_t offset  = type_sit + slot * size;
    uint8_t *p      = (uint8_t *)data;
    int     res     = 0;

    for(i = 0; i < size; i++) {
        if(ops) {
            res = Mcp2210_WriteEEProm(cur_handle, offset + i, *p++);
        }
        else {
            res = Mcp2210_ReadEEProm(cur_handle, offset + i, p++);
        }
        if(res) {
            return Mcp2210_GetLastError();
        }
    }

    return E_SUCCESS;
}

/**********************************************************************************/

/**********************************************************************************/
/* MCP2210 SPI Transfers Functions                                                */
/**********************************************************************************/
static void _spi_status_print(void)
{
    int ret = 0;
    SPI_STAUTS_S status = {0};

    ret = Mcp2210_GetSpiStatus(cur_handle, &status.SpiExtReqState, &status.SpiOwner, &status.SpiTxferState);
    myDebug("status_ret=%d, SpiExtReqState[%02x], SpiOwner[%02x], SpiTxferState[%02x]", ret,
           status.SpiExtReqState, status.SpiOwner, status.SpiTxferState);
}

static int _spi_wait_free(void)
{
    SPI_STAUTS_S status     = {0};
    uint32_t     try_count  = 100;

    while(try_count--) {
        Mcp2210_GetSpiStatus(cur_handle, &status.SpiExtReqState, &status.SpiOwner, &status.SpiTxferState);
        if(status.SpiExtReqState == 0x1 &&
           status.SpiOwner       == 0x0 &&
           status.SpiTxferState  == 0x0) {
            return E_SUCCESS;
        }
        sleep(10);
    }

    return E_ERR_SPI_TIMEOUT;
}

static int _spi_transfer(const void *send_data, uint8_t send_len, void *recv_data, uint8_t recv_len)
{
    const uint8_t   *sendbuf    = (const uint8_t *)send_data;
    uint8_t         *recvbuf    = (uint8_t *)recv_data;
    uint32_t        BaudRate    = 1000000;
    uint32_t        xferSize    = send_len > recv_len ? send_len : recv_len;

    if(_spi_wait_free()) {
        return E_ERR_SPI_TIMEOUT;
    }

    Mcp2210_xferSpiData(cur_handle, (uint8_t *)sendbuf, (uint8_t *)recvbuf, &BaudRate, &xferSize, 0);
    //DataDump("s", (uint8_t *)sendbuf, send_len);
    //DataDump("r", (uint8_t *)recvbuf, recv_len);

    return E_SUCCESS;
}
/**********************************************************************************/

/**********************************************************************************/
/* CC8531 EHIF Functions Basic Capsulation for MCP2210                            */
/**********************************************************************************/
typedef enum ehif_magic_num_e {
    EHIF_MAGIC_CMD_REQ      = 0x3,      /* 0b11 */
    EHIF_MAGIC_WRITE        = 0x8,      /* 0b1000 */
    EHIF_MAGIC_READ         = 0x9,      /* 0b1001 */
    //EHIF_MAGIC_READBC       = 0xa,      /* 0b1010 */
} EHIF_MAGIC_NAM_E;

/* SPI Data Head Defines */
typedef struct spi_head_CMD_REQ_s
{
    uint16_t    data_len    :8;
    uint16_t    cmd_type    :6;
    uint16_t    magic_num   :2;
} SPI_HEAD_CMD_REQ_S;

typedef struct spi_head_READ_WRITE_s
{
    uint16_t    data_len    :12;
    uint16_t    magic_num   :4;
} SPI_HEAD_READ_WRITE_S;

#define EHIF_HEAD_SIZE  sizeof(SPI_HEAD_CMD_REQ_S)

/* SPI Status Word Define */
typedef struct ehif_status_word_s
{
    uint16_t    evt_sr_chg          :1;
    uint16_t    evt_nwk_chg         :1;
    uint16_t    evt_ps_chg          :1;
    uint16_t    evt_vol_chg         :1;
    uint16_t    evt_spi_error       :1;
    uint16_t    evt_dsc_reset       :1;
    uint16_t    evt_dsc_tx_avail    :1;
    uint16_t    evt_dsc_rx_avail    :1;
    uint16_t    wasp_conn           :1;
    uint16_t    pwr_state           :3;
    uint16_t                        :3;
    uint16_t    cmdreq_rdy          :1;
} EHIF_STATUS_S;

typedef enum spi_cmd_req_type_e
{
    CMD_NONE                  = 0x00,
    CMD_DI_GET_CHIP_INFO      = 0x1F,
    CMD_DI_GET_DEVICE_INFO    = 0x1E,

    CMD_EHC_EVT_MASK          = 0x1A,
    CMD_EHC_EVT_CLR           = 0x19,

    CMD_NVM_DO_SCAN           = 0x08,
    CMD_NVM_DO_JOIN           = 0x09,
    CMD_NVM_GET_STATUS        = 0x0A,
    CMD_NVM_ACH_SET_USAGE     = 0x0B,
    CMD_NVM_CONTROL_ENABLE    = 0x0C,
    CMD_NVM_CONTROL_SIGNAL    = 0x0D,

    CMD_VC_GET_VOLUME         = 0x16,
    CMD_VC_SET_VOLUME         = 0x17,

    CMD_IO_GET_PIN_VAL        = 0x2A,

    CMD_NVS_GET_DATA          = 0x2B,
    CMD_NVS_SET_DATA          = 0x2C,

    CMD_RC_SET_DATA           = 0x2D,
    CMD_RC_GET_DATA           = 0x2E,


} SPI_CMD_REQ_TYPE_E;

static int CheckEndian(void)
{
    union{
        uint32_t i;
        uint8_t  s[4];
    } c;

    c.i = 0x12345678;
    return (0x12 == c.s[0]);
}

#define _SWAP16(A)  ((((uint16_t)(A) & 0xff00) >> 8) | \
                    (((uint16_t)(A) & 0x00ff) << 8))

#define _SWAP32(A)  ((((uint32_t)(A) & 0xff000000) >> 24) | \
                    (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
                    (((uint32_t)(A) & 0x0000ff00) << 8)  | \
                    (((uint32_t)(A) & 0x000000ff) << 24))

#define Swap16(x) (CheckEndian() ? (x) : _SWAP16(x))
#define Swap32(x) (CheckEndian() ? (x) : _SWAP32(x))

static int _basic_operation(EHIF_MAGIC_NAM_E magic,
                        uint16_t cmd, uint16_t *len, void *data, EHIF_STATUS_S *status)
{
    SPI_HEAD_CMD_REQ_S      cmd_head    = {0};
    SPI_HEAD_READ_WRITE_S   rw_head     = {0};
    void                    *phead      = NULL;
    uint8_t                 *sendbuf    = (uint8_t *)malloc(EHIF_HEAD_SIZE + *len);
    uint8_t                 *recvbuf    = (uint8_t *)malloc(EHIF_HEAD_SIZE + *len);
    int                     ret;

    cmd_head.cmd_type   = cmd;
    cmd_head.data_len   = rw_head.data_len  = *len;
    cmd_head.magic_num  = rw_head.magic_num = magic;

    memset(status, 0, sizeof(EHIF_STATUS_S));

    phead = (magic == EHIF_MAGIC_CMD_REQ) ? (void *)&cmd_head : (void *)&rw_head;
    *(uint16_t *)phead = Swap16(*(uint16_t *)phead);

    memset(sendbuf, 0, EHIF_HEAD_SIZE + *len);
    memset(recvbuf, 0, EHIF_HEAD_SIZE + *len);
    memcpy(sendbuf, phead, EHIF_HEAD_SIZE);

    switch(magic) {
    case EHIF_MAGIC_CMD_REQ:
    case EHIF_MAGIC_WRITE:
        if(*len) {
            memcpy(sendbuf + EHIF_HEAD_SIZE, data, *len);
        }
        ret = _spi_transfer(sendbuf, EHIF_HEAD_SIZE + *len, recvbuf, EHIF_HEAD_SIZE);
        break;
    case EHIF_MAGIC_READ:
        ret = _spi_transfer(sendbuf, EHIF_HEAD_SIZE, recvbuf, EHIF_HEAD_SIZE + *len);
        if(*len) {
            memcpy(data, recvbuf + EHIF_HEAD_SIZE, *len);
        }
        break;
    }

    *(uint16_t *)recvbuf = Swap16(*(uint16_t *)recvbuf);
    memcpy(status, recvbuf, EHIF_HEAD_SIZE);
    free(sendbuf);
    free(recvbuf);

    return ret;
}

inline static int basic_CMD_REQ(uint16_t cmd, uint16_t len, void *pdata, EHIF_STATUS_S *pstatus)
{
    return _basic_operation(EHIF_MAGIC_CMD_REQ, cmd, &len, pdata, pstatus);
}

inline static int basic_WRITE(uint16_t len, void *pdata, EHIF_STATUS_S *pstatus)
{
    return _basic_operation(EHIF_MAGIC_WRITE, CMD_NONE, &len, (pdata), (pstatus));
}

inline static int basic_READ(uint16_t len, void *pdata, EHIF_STATUS_S *pstatus)
{
    return _basic_operation(EHIF_MAGIC_READ, CMD_NONE, &len, (pdata), (pstatus));
}
/* MCP2210上不能控制不了SPI引脚，每次调用API传输函数时自身进行了处理，一次数据不能分开传输，READBC无法实现 */

inline static int basic_GET_STATUS(EHIF_STATUS_S *pstatus)
{
    uint16_t l = 0;
    return _basic_operation(EHIF_MAGIC_WRITE, 0, &l, 0, (pstatus));
}
/**********************************************************************************/

/**********************************************************************************/
/* CC8531 EHIF Ctrl Functions                                                     */
/**********************************************************************************/
#define VOLUME_IS_IN_VOL_OUTPUT     0
#define VOLUME_IS_IN_VOL_INPUT      1

#define VOLUME_IS_LOCAL_REMOTE      0
#define VOLUME_IS_LOCAL_LOCAL       1
typedef struct cc85xx_device_info_s
{
    uint32_t    device_id;
    uint32_t    manf_id;
    uint32_t    prod_id;
} CC85XX_DEV_INFO_S;

typedef struct cc85xx_set_volume_s
{
    uint8_t     is_local    :1;
    uint8_t     is_in_vol   :1;
    uint8_t                 :6;
    uint8_t     log_channel :4;
    uint8_t     set_op      :2;
    uint8_t     mute_op     :2;
    uint16_t    volume      :16;
} CC85XX_SET_VOLUME_S;

typedef struct cc85xx_get_volume_s
{
    uint8_t     is_local    :1;
    uint8_t     is_in_vol   :1;
    uint8_t                 :6;
} CC85XX_GET_VOLUME_S;

typedef struct cc85xx_nwm_get_status_s
{
    uint8_t     byte0_to_4[5];
    uint8_t     dev_data[16 * MIC_MAX_NUM];
} CC85XX_NWM_GET_STATUS_S;

typedef struct rc_get_data_head_s
{
    uint8_t     cmd_count   :3;
    uint8_t     keyb_count  :3;
    uint8_t     ext_sel     :2;
} RC_GET_DATA_HEAD_S;

typedef struct rc_get_data_s
{
    RC_GET_DATA_HEAD_S  head;
    uint8_t             data[12];
} RC_GET_DATA_S;

#define RES_ASSERT(func) \
{                               \
    int res = (func);           \
    if(res != E_SUCCESS) {      \
        return res;             \
    }                           \
    (res);                      \
}

static int ehif_DI_GET_DEVICE_INFO(CC85XX_DEV_INFO_S *dev_info)
{
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_DI_GET_DEVICE_INFO, 0, NULL, &status));
    RES_ASSERT(basic_READ(sizeof(CC85XX_DEV_INFO_S), (uint8_t *)dev_info, &status));

    dev_info->device_id = Swap32(dev_info->device_id);
    dev_info->manf_id   = Swap32(dev_info->manf_id);
    dev_info->prod_id   = Swap32(dev_info->prod_id);

    return E_SUCCESS;
}

static int ehif_EHC_EVT_CLR(uint8_t val)
{
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_EHC_EVT_CLR, sizeof(uint8_t), &val, &status));

    return E_SUCCESS;
}

static int ehif_NVS_GET_DATA(uint8_t slot, void *data)
{
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_NVS_GET_DATA, sizeof(uint8_t), &slot, &status));
    RES_ASSERT(basic_READ(sizeof(uint32_t), data, &status));

    //*(uint32_t *)nwm_id = Swap32(*(uint32_t *)nwm_id);

    return E_SUCCESS;
}

static int ehif_NVS_SET_DATA(uint8_t slot, uint32_t data)
{
    uint8_t         set_data[5] = {slot};
    EHIF_STATUS_S   status = {0};
    //uint32_t        tmp = Swap32(data);

    memcpy(&set_data[1], &data, sizeof(uint32_t));

    RES_ASSERT(basic_CMD_REQ(CMD_NVS_SET_DATA, sizeof(set_data), set_data, &status));

    return E_SUCCESS;
}

static int ehif_IO_GET_PIN_VAL(uint16_t *gpio_val)
{
    EHIF_STATUS_S   status = {0};
    uint8_t         recvbuf[4] = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_IO_GET_PIN_VAL, 0, NULL, &status));
    RES_ASSERT(basic_READ(sizeof(recvbuf), recvbuf, &status));

    memcpy(gpio_val, &recvbuf[2], sizeof(uint16_t));
    *gpio_val = Swap16(*gpio_val);

    return E_SUCCESS;
}

static int ehif_NWM_CONTROL_ENABLE(uint8_t enable)
{
    uint8_t         sendbuf[2] = {0, enable};
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_NVM_CONTROL_ENABLE, sizeof(sendbuf), sendbuf, &status));

    return E_SUCCESS;
}

static int ehif_NWM_CONTROL_SIGNAL(void)
{
    uint8_t         sendbuf[2] = {0, 1};
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_NVM_CONTROL_SIGNAL, sizeof(sendbuf), sendbuf, &status));

    return E_SUCCESS;
}

static int ehif_CMD_NVM_GET_STATUS(CC85XX_NWM_GET_STATUS_S *nwm_status)
{
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_NVM_GET_STATUS, 0, NULL, &status));
    RES_ASSERT(basic_READ(sizeof(CC85XX_NWM_GET_STATUS_S), nwm_status, &status));

    return E_SUCCESS;
}

static int ehif_VC_SET_VOLUME(CC85XX_SET_VOLUME_S *set_volume)
{
    EHIF_STATUS_S   status = {0};
    void            *pdata = (void *)set_volume;

    set_volume->volume = Swap16(set_volume->volume);
    RES_ASSERT(basic_CMD_REQ(CMD_VC_SET_VOLUME, sizeof(CC85XX_SET_VOLUME_S), pdata, &status));

    return E_SUCCESS;
}

static int ehif_VC_GET_VOLUME(CC85XX_GET_VOLUME_S *get_volume, uint16_t *volume)
{
    EHIF_STATUS_S   status = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_VC_GET_VOLUME, 1, get_volume, &status));
    RES_ASSERT(basic_READ(sizeof(uint16_t), volume, &status));

    *volume = Swap16(*volume);

    return E_SUCCESS;
}

static int ehif_RC_GET_DATA(uint8_t slot, uint8_t *data, uint8_t *count)
{
    RC_GET_DATA_S   get_data    = {0};
    EHIF_STATUS_S   status      = {0};

    RES_ASSERT(basic_CMD_REQ(CMD_RC_GET_DATA, sizeof(slot), &slot, &status));
    RES_ASSERT(basic_READ(sizeof(RC_GET_DATA_S), &get_data, &status));

    *count = get_data.head.cmd_count;
    if(*count > 12) {
        return E_ERR_CMD_FAILED;
    }
    memcpy(data, get_data.data, *count);

    return E_SUCCESS;
}

static int _ehif_ctrl_mic_output_volume(uint8_t mute_op, uint8_t set_op, uint8_t log_cnn, uint16_t volume)
{
    CC85XX_SET_VOLUME_S set_volume  = {VOLUME_IS_LOCAL_LOCAL, VOLUME_IS_IN_VOL_OUTPUT};
    set_volume.log_channel  = log_cnn;
    set_volume.mute_op      = mute_op;
    set_volume.set_op       = set_op;
    set_volume.volume       = volume;

    ehif_VC_SET_VOLUME(&set_volume);

    return E_SUCCESS;
}

#define ehif_ctrl_mic_output_volume_init() \
    _ehif_ctrl_mic_output_volume(2, 1, 0, 0)
#define ehif_ctrl_mic_output_volume_absolute(cnn, abs) \
    _ehif_ctrl_mic_output_volume(2, 3, (cnn), (abs))

/**********************************************************************************/

/**********************************************************************************/
/* Button Interface Functions                                                     */
/**********************************************************************************/
typedef enum button_status_e
{
    BUTTON_NORMAL = 0,
    BUTTON_PRESSED,
    BUTTON_FOCUSED,
} BUTTON_STATUS_E;

#define SHACK_OLD_PRESS  1
#define SHACK_OLD_NORMAL 2

/* MCP命令响应速度太慢，完全没必要做抖动检测了 */
#define SHACK_INTERVAL   1
#define PRESSED_INTERVAL 3
#define FOCUSED_INTERVAL 20

typedef struct button_s
{
    uint8_t     status  :5;
    uint8_t     shack   :2;
    uint8_t     avtice  :1;
    uint16_t    duration;
} BUTTON_S;

static int CheckButtonActive(BUTTON_S *button, uint8_t IsPress)
{
    /* shack process */
    if(button->shack) {
        if((button->shack == SHACK_OLD_PRESS  && IsPress) ||
           (button->shack == SHACK_OLD_NORMAL && !IsPress)) {
            button->duration++;
        }
        else {
            button->shack = IsPress ? SHACK_OLD_PRESS : SHACK_OLD_NORMAL;
            button->duration = 0;
        }

        if(button->duration >= SHACK_INTERVAL) {
            button->shack     = 0;
            button->duration  = 0;
            if(!button->status && IsPress) {
                button->status = BUTTON_PRESSED;
                button->avtice = 1;
            }
            else if(button->status && !IsPress) {
                button->status = BUTTON_NORMAL;
            }
        }
        return button->avtice;
    }

    switch(button->status) {
    case BUTTON_NORMAL:
        if(IsPress) {
            button->shack = SHACK_OLD_PRESS;
        }
        break;
    case BUTTON_PRESSED:
    case BUTTON_FOCUSED:
        if(!IsPress) {
            button->shack = SHACK_OLD_NORMAL;
            button->duration  = 0;
        }
        else {
            button->duration++;
            if(button->duration >= (button->status == BUTTON_PRESSED ? FOCUSED_INTERVAL : PRESSED_INTERVAL)) {
                button->duration = 0;
                button->status = BUTTON_FOCUSED;
                button->avtice = 1;
            }
        }
        break;
    }
    return button->avtice;
}
/**********************************************************************************/
/* Vocal System Control Functions                                                 */
/**********************************************************************************/
BUTTON_S sPairingButton;

#define SYS_CON_ASSERT(f, s) \
{                                   \
    int res = (f);                  \
    if(res != E_SUCCESS) {          \
        (s)->Spi_Conn = 0;          \
        (s)->Net_Enable = 0;        \
        return E_ERR_SPI_TIMEOUT;   \
    }                               \
    (s)->Spi_Conn = 1;              \
    (res);                          \
}

typedef struct dev_volume_s
{
    uint32_t    volume  :15;
    uint32_t    mute    :1;
} DEV_VOLUME_S;

static int _Rom_Record_Alone(MIC_DEVINFO_S *dev)
{
    int res = 0;
    DEV_VOLUME_S volume = {0};

    volume.mute   = dev->mute;
    volume.volume = dev->volume & 0x7fff;

    res = _MCP2210_Record(RECORD_SET, SIT_DEVICE_ID, dev->ram_slot - 1, &dev->device_id);
    if(res) {
        return res;
    }

    return _MCP2210_Record(RECORD_SET, SIT_VOLUME, dev->ram_slot - 1, &volume);
}

static int _Rom_Record_Sync(VOCAL_SYS_STATUS_S *sys_status)
{
    int i, j;
    uint32_t     rom_dev_id = 0;
    DEV_VOLUME_S rom_volume = {0};
    uint8_t      rom_free[MIC_MAX_NUM] = {0};

    for(i = 0; i < MIC_MAX_NUM; i++) {
        SYS_CON_ASSERT(_MCP2210_Record(RECORD_GET, SIT_DEVICE_ID, i, &rom_dev_id), sys_status);
        SYS_CON_ASSERT(_MCP2210_Record(RECORD_GET, SIT_VOLUME, i, &rom_volume), sys_status);
        myDebug("RAM[%d]:id=0x%08x, volume=0x%04x, mute=%d",
               i, rom_dev_id, rom_volume.volume, rom_volume.mute);
        if(!rom_dev_id) {
            rom_free[i] = true;
            continue;
        }
        for(j = 0; j < MIC_MAX_NUM; j++) {
            if(rom_dev_id == (uint32_t)(sys_status->Mic_Info[j].device_id)) {
                sys_status->Mic_Info[j].ram_slot = i + 1;
                sys_status->Mic_Info[j].mute     = rom_volume.mute;
                sys_status->Mic_Info[j].volume   = rom_volume.volume & 0x7fff;
                break;
            }
        }
        if(j == MIC_MAX_NUM) {
            rom_free[i] = true;
        }
    }

    for(j = 0; j < MIC_MAX_NUM; j++) {
        if(!sys_status->Mic_Info[j].device_id || sys_status->Mic_Info[j].ram_slot) {
            continue;
        }
        for(i = 0; i < MIC_MAX_NUM; i++) {
            if(rom_free[i]) {
                myDebug("dev[%d]:add new ram[%d]", j, i);
                sys_status->Mic_Info[j].ram_slot = i + 1;
                sys_status->Mic_Info[j].mute     = 0;
                sys_status->Mic_Info[j].volume   = 320;
                rom_free[i] = false;
                SYS_CON_ASSERT(_Rom_Record_Alone(&sys_status->Mic_Info[j]), sys_status);
                break;
            }
        }
    }

    return E_SUCCESS;
}

static int16_t _Volume_Level_To_Db(int16_t lv) {
    if(lv >= 100) {
        lv = 100;
    }
    if(lv <= 0) {
        lv = 0;
    }
    if(lv > 68) {
        return 368 + lv - 68;
    }
    if(lv > 44) {
        return 320 + (lv - 44)*2;
    }
    if(lv > 24) {
        return 240 + (lv - 24)*4;
    }
    if(lv > 8) {
        return 112 + (lv - 8)*8;
    }
    return lv * 14;
}

static int _Volume_Set_Alone(MIC_DEVINFO_S *dev)
{
    if(dev->mute || dev->volume == 0) {
        return ehif_ctrl_mic_output_volume_absolute(dev->ach, 0xfc00);
    }

    return ehif_ctrl_mic_output_volume_absolute(dev->ach, 0xfe70 + _Volume_Level_To_Db(dev->volume));
}

static int _Volume_Updata(VOCAL_SYS_STATUS_S *sys_status)
{
    int i;

    RES_ASSERT(ehif_ctrl_mic_output_volume_init());
    for(i = 0; i < MIC_MAX_NUM; i++) {
        if(sys_status->Mic_Info[i].device_id) {
            RES_ASSERT(_Volume_Set_Alone(&sys_status->Mic_Info[i]));
        }
    }

    return E_SUCCESS;
}

static uint8_t _GetAuidoChannel(uint16_t ach_used)
{
    if(ach_used >> 12 & 0x1) {
        return 12;
    }
    if(ach_used >> 12 & 0x2) {
        return 13;
    }
    return 16;
}

static int _Updata_NetConfig(VOCAL_SYS_STATUS_S *sys_status)
{
    CC85XX_NWM_GET_STATUS_S nwm_status = {0};
    int     i;
    void    *id  = NULL;
    void    *ach_used = NULL;
    uint8_t updata_data_abnormal = false;

    myDebug("updata dev info");
    memset(sys_status->Mic_Info, 0, sizeof(sys_status->Mic_Info));
    sleep(200);
    SYS_CON_ASSERT(ehif_CMD_NVM_GET_STATUS(&nwm_status), sys_status);
    for(i = 0; i < MIC_MAX_NUM; i++) {
        id       = &nwm_status.dev_data[0 + i*16];
        ach_used = &nwm_status.dev_data[12 + i*16];
        if(*(uint32_t *)id) {
            if(0 == *(uint16_t *)ach_used) {
                updata_data_abnormal = true;
                continue;
            }
            sys_status->Mic_Info[i].device_id = Swap32(*(uint32_t *)id);
            sys_status->Mic_Info[i].ach       = _GetAuidoChannel(Swap16(*(uint16_t *)ach_used));
            sys_status->Mic_Info[i].slot      = (nwm_status.dev_data[15+i*16] >> 1) & 0x7;
            myDebug("i=%d, slot=%d, ach=%d, device_id=0x%08x", i,
                   sys_status->Mic_Info[i].slot, sys_status->Mic_Info[i].ach, sys_status->Mic_Info[i].device_id);
        }
    }
    if(updata_data_abnormal) {
        SYS_CON_ASSERT(ehif_NWM_CONTROL_ENABLE(0), sys_status);
        sleep(10);
        SYS_CON_ASSERT(ehif_NWM_CONTROL_ENABLE(1), sys_status);
    }
    else {
        SYS_CON_ASSERT(_Rom_Record_Sync(sys_status), sys_status);
        SYS_CON_ASSERT(_Volume_Updata(sys_status), sys_status);
        sys_status->Net_Updata = 1;
        SYS_CON_ASSERT(ehif_EHC_EVT_CLR(1<<1), sys_status);
    }

    return E_SUCCESS;
}

static int _Remote_Control(MIC_DEVINFO_S *mic)
{
    switch(mic->cmd) {
    case OUTPUT_VOLUME_INCREMENT:
        if(mic->volume + 5 >= 100) {
            mic->volume = 100;
        }
        else mic->volume += 5;
        myDebug("========> db=%d", mic->volume);
        break;
    case OUTPUT_VOLUME_DECREASE:
        (mic->volume - 5 <= 0) ? mic->volume = 0 : mic->volume -= 5;
        myDebug("========> db=%d", mic->volume);
        break;
    case OUTPUT_VOLUME_MUTE:
        mic->mute = mic->mute ? false : true;
        myDebug("volume mute=%d", mic->mute);
        break;
    }
    _Rom_Record_Alone(mic);
    _Volume_Set_Alone(mic);

    return E_SUCCESS;
}

static int _mic_rc_cmd_process(VOCAL_SYS_STATUS_S *sys_status)
{
    uint8_t rc_data[12] = {0};
    uint8_t rc_count;
    int     i;
    static uint8_t  cmd_record[MIC_MAX_NUM] = {0};
    static uint8_t  cnt_record[MIC_MAX_NUM] = {0};
    static int times = 0;

    for(i = 0; i < MIC_MAX_NUM; i++) {
        if(sys_status->Mic_Info[i].device_id) {
            SYS_CON_ASSERT(ehif_RC_GET_DATA(sys_status->Mic_Info[i].slot, rc_data, &rc_count), sys_status);

            if(rc_count) {
                if(!cnt_record[i] || cmd_record[i] != rc_data[0]) {
                    myDebug("[%d]cmd=%d", times, rc_data[0]);
                    sys_status->Mic_Info[i].cmd = rc_data[0];
                    _Remote_Control(&sys_status->Mic_Info[i]);
                    sys_status->Vol_Updata = 1;
                }
                times++;
            }
            cnt_record[i] = rc_count;
            cmd_record[i] = rc_data[0];
        }
    }

    return E_SUCCESS;
}

static int _Vocal_Sys_Process(VOCAL_SYS_STATUS_S *sys_status, EHIF_STATUS_S *ehif_status)
{
    static int i = 0;
    if(CheckButtonActive(&sPairingButton, sys_status->Paring_Key)) {
        sPairingButton.avtice = 0;
        if(sPairingButton.status != BUTTON_FOCUSED && ehif_status->cmdreq_rdy) {
            myDebug("Paring %d", i++);
            SYS_CON_ASSERT(ehif_NWM_CONTROL_SIGNAL(), sys_status);
        }
    }

    if(ehif_status->cmdreq_rdy) {
        SYS_CON_ASSERT(_mic_rc_cmd_process(sys_status), sys_status);
    }

    return E_SUCCESS;
}

void Try_To_close_Net(void)
{
    ehif_NWM_CONTROL_ENABLE(0);
}

#define BIT_ISSET(a, s)     (((a) >> (s)) & 0x1)
int Vocal_Sys_updata_Process(VOCAL_SYS_STATUS_S *sys_status)
{
    EHIF_STATUS_S       ehif_status  = {0};
    uint16_t            gpio_val = 0;

    /* 1 updata sys_status */
    static int time = 0;
    if(time == 10) {
        SYS_CON_ASSERT(basic_GET_STATUS(&ehif_status), sys_status);
        time = 0;
    }
    else {
        time++;
    }


    if(!sys_status->Net_Enable) {
        SYS_CON_ASSERT(ehif_NWM_CONTROL_ENABLE(1), sys_status);
    #if 0
        int i = 0;
        for(i = 0; i < 24; i++) {
            Mcp2210_WriteEEProm(cur_handle, i, 0);
        }
    #endif
        sys_status->Net_Enable = 1;
    }

    if(ehif_status.cmdreq_rdy) {
        if(ehif_status.evt_nwk_chg) {
            SYS_CON_ASSERT(_Updata_NetConfig(sys_status), sys_status);
        }
        SYS_CON_ASSERT(ehif_IO_GET_PIN_VAL(&gpio_val), sys_status); /* 26ms */
        sys_status->Paring_Key = BIT_ISSET(~gpio_val, 2);
    }

    /* 2 process */
    SYS_CON_ASSERT(_Vocal_Sys_Process(sys_status, &ehif_status), sys_status); /* 58ms */

    return E_SUCCESS;
}

#ifdef __cplusplus
}
#endif
