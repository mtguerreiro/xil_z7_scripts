
#ifndef GET_UID_H_
#define GET_UID_H_

#include "stdint.h"

#define GET_UID_ERROR_BUF_INVALID_PARAM     -1
#define GET_UID_ERROR_BUF_SIZE              -2
#define GET_UID_ERROR_QSPI_LOOK_UP_CFG      -3
#define GET_UID_ERROR_QSPI_INIT             -4
#define GET_UID_ERROR_QSPI_SELF_TEST        -5
#define GET_UID_ERROR_QSPI_ADDRMODE_READ    -6
#define GET_UID_ERROR_QSPI_UID_READ         -7
#define GET_UID_ERROR_INT_BUF_SIZE          -8

typedef struct
{
    uint8_t addr_mode_rd_cmd;
    uint8_t addr_mode_mask;
    uint8_t addr_mode_3b_value;

    uint8_t uid_n_dummy_3b;
    uint8_t uid_n_dummy_4b;
    uint8_t uid_rd_cmd;
    uint8_t uid_size;
} get_uid_config_t;

int32_t get_uid(get_uid_config_t *cfg, uint8_t *buffer, uint32_t size);

#endif /* GET_UID_H_ */
