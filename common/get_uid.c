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
/*--------------------------------- Defines ---------------------------------*/
//=============================================================================
// #define QSPI_W25Q256JV_CMD_RD_SR3           0x15
// #define QSPI_W25Q256JV_CMD_RD_UID           0x4B
// #define QSPI_W25Q256JV_UID_SIZE_BYTES       8
//
// #define QSPI_W25Q256JV_SR3_ADS_MASK         0x01
// #define QSPI_W25Q256JV_SR3_ADS_3B_MASK      0
//
// #define QSPI_W25Q256JV_UID_NDUMMY_ADS_3B    4
// #define QSPI_W25Q256JV_UID_NDUMMY_ADS_4B    5
//
// #define QSPI_W25Q256JV_BUF_SIZE_BYTES       16
//=============================================================================

//=============================================================================
/*--------------------------------- Globals ---------------------------------*/
//=============================================================================

//=============================================================================

//=============================================================================
/*---------------------------------- Task -----------------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
int32_t get_uid(get_uid_config_t *cfg, uint8_t *buffer, uint32_t size){

    int32_t status;
    uint8_t buf[96] = {0};
    uint8_t n_dummys;
    uint8_t uid_read_size;

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

    buf[0] = cfg->addr_mode_rd_cmd;
    status = XQspiPs_PolledTransfer(&qspi_instance, buf, buf, 2);
    if (status != XST_SUCCESS)
        return GET_UID_ERROR_QSPI_ADDRMODE_READ;

    addr_mode = buf[1] & cfg->addr_mode_mask;

    if( addr_mode == cfg->addr_mode_mask )
        n_dummys = cfg->uid_n_dummy_3b;
    else
        n_dummys = cfg->uid_n_dummy_4b;


    /* Reads UID */

    memset(buf, 0, sizeof(buf));
    buf[0] = cfg->uid_rd_cmd;
    uid_read_size = 1 + n_dummys + cfg->uid_size;

    if( sizeof(buf) < uid_read_size )
        return GET_UID_ERROR_INT_BUF_SIZE;

    status = XQspiPs_PolledTransfer(&qspi_instance, buf, buf, uid_read_size);
    if (status != XST_SUCCESS)
        return GET_UID_ERROR_QSPI_UID_READ;

    uid_offset = 1 + n_dummys;
    memcpy(buffer, &buf[uid_offset], cfg->uid_size);

    return cfg->uid_size;
}
//-----------------------------------------------------------------------------
//=============================================================================
