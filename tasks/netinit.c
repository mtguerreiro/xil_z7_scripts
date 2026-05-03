
//=============================================================================
/*-------------------------------- Includes ---------------------------------*/
//=============================================================================
#include "netinit.h"

/* Kernel */
#include "FreeRTOS.h"
#include "task.h"

/* Device and drivers */
#include "xparameters.h"
#include "netif/xadapter.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xil_io.h"
#include "xqspips.h"

/* lwip */
#include "lwipopts.h"
#include "lwip/dhcp.h"

/* Logging config */
#include "clogging/logging_levels.h"

#ifndef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME    "NET_INIT"
#endif

#ifndef LIBRARY_LOG_LEVEL
#define LIBRARY_LOG_LEVEL    LOG_DEBUG
#endif
#include "clogging/logging_stack.h"
//=============================================================================

//=============================================================================
/*--------------------------------- Defines ---------------------------------*/
//=============================================================================
/* Ethernet settings */
#define NETINIT_PLAT_EMAC_BASEADDR                  XPAR_XEMACPS_0_BASEADDR
#define NETINIT_CONFIG_THREAD_STACK_SIZE_DEFAULT    2048
#define NETINIT_CONFIG_THREAD_PRIO_DEFAULT          DEFAULT_THREAD_PRIO
//=============================================================================


//=============================================================================
/*--------------------------------- Globals ---------------------------------*/
//=============================================================================
struct netif servernetif;
//=============================================================================

//=============================================================================
/*-------------------------------- Prototypes -------------------------------*/
//=============================================================================
/**
 * @brief Initializes socket and updates DHCP timer.
 */
static void netinitNetworkThread(void *param);

/**
 * @brief Prints IP settings
 */
static void netinitPrintIPSettings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);

extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
void lwip_init();

//=============================================================================

//=============================================================================
/*---------------------------------- Task -----------------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
void netinit(void *param){

    netinitParams_t *cfg = (netinitParams_t *)param;
    struct netif *netif;

    int mscnt = 0;

    /* initialize lwIP before calling sys_thread_new */
    lwip_init();

    /* any thread using lwIP should be created using sys_thread_new */
    sys_thread_new(
        "netinitNWThread", netinitNetworkThread, (void *)cfg->mac,
        NETINIT_CONFIG_THREAD_STACK_SIZE_DEFAULT,
        NETINIT_CONFIG_THREAD_PRIO_DEFAULT
    );

    netif = &servernetif;

    while (1) {

        vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);

        if (netif->ip_addr.addr) {
            LogInfo(( "DHCP request success" ));
            break;
        }

        mscnt += DHCP_FINE_TIMER_MSECS;

        if (mscnt >= DHCP_COARSE_TIMER_SECS * 2000) {
            LogError(( "DHCP request timed out. Configuring default ip of 192.168.0.10" ));
            IP4_ADDR(&(netif->ip_addr),  192, 168, 0, 10);
            IP4_ADDR(&(netif->netmask), 255, 255, 255,  0);
            IP4_ADDR(&(netif->gw),  192, 168, 0, 1);
            break;
        }
    }

    netinitPrintIPSettings(&(netif->ip_addr), &(netif->netmask), &(netif->gw));

    if( cfg->onInit)
        cfg->onInit();

    vTaskDelete(NULL);
}
//-----------------------------------------------------------------------------
//=============================================================================

//=============================================================================
/*---------------------------- Static functions -----------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
static void netinitNetworkThread(void *param){

    uint8_t *p = (uint8_t *)param;
    ip_addr_t ipaddr, netmask, gw;
    struct netif *netif;
    uint8_t mac[6] = {0x02, 0x11, 0x13, 0x57, 0x3a, 0xf3};

    if( p ){
        uint32_t k;
        for(k = 0; k < 6; k++) mac[k] = p[k];
    }

    LogInfo((
        "Board MAC: %02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    ));

    int mscnt = 0;

    netif = &servernetif;

    LogInfo(( "Initializing network settings" ));

    ipaddr.addr = 0;
    gw.addr = 0;
    netmask.addr = 0;

    /* Add network interface to the netif_list, and set it as default */
    if (!xemac_add(netif, &ipaddr, &netmask, &gw, mac, NETINIT_PLAT_EMAC_BASEADDR)) {
        LogError(( "Error adding N/W interface" ));
        return;
    }

    netif_set_default(netif);

    /* specify that the network if is up */
    netif_set_up(netif);

    /* start packet receive thread - required for lwIP operation */
    sys_thread_new(
        "xemacif_input_thread", (void(*)(void*))xemacif_input_thread, netif,
        NETINIT_CONFIG_THREAD_STACK_SIZE_DEFAULT,
        NETINIT_CONFIG_THREAD_PRIO_DEFAULT
    );

    dhcp_start(netif);
    while (1) {
        vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
        dhcp_fine_tmr();
        mscnt += DHCP_FINE_TIMER_MSECS;
        if (mscnt >= DHCP_COARSE_TIMER_SECS*1000) {
            dhcp_coarse_tmr();
            mscnt = 0;
        }
    }

    return;
}
//-----------------------------------------------------------------------------
static void netinitPrintIPSettings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw){

    LogInfo(( "Board IP : %d.%d.%d.%d", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip) ));
    LogInfo(( "Netmask  : %d.%d.%d.%d", ip4_addr1(mask), ip4_addr2(mask), ip4_addr3(mask), ip4_addr4(mask) ));
    LogInfo(( "Gateway  : %d.%d.%d.%d", ip4_addr1(gw), ip4_addr2(gw), ip4_addr3(gw), ip4_addr4(gw) ));
}
//-----------------------------------------------------------------------------
//=============================================================================
