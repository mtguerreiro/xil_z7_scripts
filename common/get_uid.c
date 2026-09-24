//=============================================================================
/*-------------------------------- Includes ---------------------------------*/
//=============================================================================
#include "get_uid.h"

#include "stdio.h"

/* Device and drivers */
#include "xparameters.h"
#include "xqspips.h"
//=============================================================================

//=============================================================================
/*---------------------------------- Task -----------------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
int32_t get_uid(get_uid_config_t *cfg, uint8_t *buffer, uint32_t size){

    int32_t status;
    uint8_t buf[96] = {0};
    uint8_t n_dummys;
    uint32_t uid_offset;
    uint32_t addr_mode;
    static XQspiPs qspi_instance;
    XQspiPs_Config *qspi_config = 0;

    if( (cfg == 0) || (buffer == 0) )
        return GET_UID_ERROR_BUF_INVALID_PARAM;

    if( size < cfg->uid_size )
        return GET_UID_ERROR_BUF_SIZE;

    /* QSPI initialization */
    qspi_config = XQspiPs_LookupConfig(XPAR_XQSPIPS_0_BASEADDR);
    if( qspi_config == 0 )
        return GET_UID_ERROR_QSPI_LOOK_UP_CFG;

    status = XQspiPs_CfgInitialize(&qspi_instance, qspi_config, qspi_config->BaseAddress);
    if( status != XST_SUCCESS )
        return GET_UID_ERROR_QSPI_INIT;

    status = XQspiPs_SelfTest(&qspi_instance);
    if (status != XST_SUCCESS)
        return GET_UID_ERROR_QSPI_SELF_TEST;

    XQspiPs_SetClkPrescaler(&qspi_instance, XQSPIPS_CLK_PRESCALE_8);
    XQspiPs_SetOptions(
        &qspi_instance,
        XQSPIPS_FORCE_SSELECT_OPTION | XQSPIPS_MANUAL_START_OPTION | XQSPIPS_HOLD_B_DRIVE_OPTION
    );

    XQspiPs_SetSlaveSelect(&qspi_instance);

    /* Determines address mode */
    memset(buf, 0, sizeof(buf));
    buf[0] = cfg->addr_mode_rd_cmd;
    status = XQspiPs_PolledTransfer(&qspi_instance, buf, buf, 2);
    if (status != XST_SUCCESS)
        return GET_UID_ERROR_QSPI_ADDRMODE_READ;

    addr_mode = buf[1] & cfg->addr_mode_mask;

    if( addr_mode == cfg->addr_mode_3b_value )
        n_dummys = cfg->uid_n_dummy_3b;
    else
        n_dummys = cfg->uid_n_dummy_4b;

    if( sizeof(buf) < (1U + n_dummys + cfg->uid_size) )
        return GET_UID_ERROR_INT_BUF_SIZE;

    /* Reads UID */
    buf[0] = cfg->uid_rd_cmd;
    status = XQspiPs_PolledTransfer(&qspi_instance, buf, buf, sizeof(buf));
    if (status != XST_SUCCESS)
        return GET_UID_ERROR_QSPI_UID_READ;

    uid_offset = 1 + n_dummys;
    memcpy(buffer, &buf[uid_offset], cfg->uid_size);

    return cfg->uid_size;
}
//-----------------------------------------------------------------------------
//=============================================================================
