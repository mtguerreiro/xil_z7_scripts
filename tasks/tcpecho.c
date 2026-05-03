
//=============================================================================
/*-------------------------------- Includes ---------------------------------*/
//=============================================================================
#include "tcpecho.h"

/* Kernel */
#include "FreeRTOS.h"
#include "task.h"

/* lwip */
#include "netif/xadapter.h"
#include "lwip/sockets.h"

/* Logging config */
#include "clogging/logging_levels.h"

#ifndef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME    "TCP_ECHO"
#endif

#ifndef LIBRARY_LOG_LEVEL
#define LIBRARY_LOG_LEVEL    LOG_DEBUG
#endif
#include "clogging/logging_stack.h"
//=============================================================================

//=============================================================================
/*-------------------------------- Prototypes -------------------------------*/
//=============================================================================
static void tcpechoProcess(void *param);
//=============================================================================

//=============================================================================
/*---------------------------------- Task -----------------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
void tcpecho(void *param){

    (void)param;
    int sock, new_sd;
    int size;

    struct sockaddr_in address, remote;

    memset(&address, 0, sizeof(address));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        LogError(( "Failed to initialize the socket... tcp echo task will be deleted" ));
        vTaskDelete(NULL);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(TCP_ECHO_CONFIG_SERVER_PORT);
    address.sin_addr.s_addr = INADDR_ANY;


    if (lwip_bind(sock, (struct sockaddr *)&address, sizeof (address)) < 0){
        LogError(( "Failed to bind... tcp echo task will be deleted" ));
        vTaskDelete(NULL);
    }

    lwip_listen(sock, 0);

    size = sizeof(remote);

    while (1) {
        if ((new_sd = lwip_accept(sock, (struct sockaddr *)&remote, (socklen_t *)&size)) > 0) {
            ip_addr_t *ip = (ip_addr_t *)&remote.sin_addr;
            LogInfo(( "Received connection from %d.%d.%d.%d", ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip) ));
            sys_thread_new(
                "tcpechoProcessThread", tcpechoProcess,
                (void*)new_sd,
                TCP_ECHO_CONFIG_TASK_STACK_SIZE,
                TCP_ECHO_CONFIG_TASK_PRIO
            );
        }
    }
}
//-----------------------------------------------------------------------------
//=============================================================================

//=============================================================================
/*---------------------------- Static functions -----------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
static void tcpechoProcess(void *param){

    int sd = (int)param;
    char buffer[TCP_ECHO_CONFIG_ECHO_BUF_SIZE_BYTES];
    int32_t n;

    n = lwip_read(sd, buffer, sizeof(buffer));
    if(n < 0 ){
        LogError(( "Error reading from socket (error %d)", (int)n ));
        LogError(( "Socket will be closed" ));
        lwip_close(sd);
        vTaskDelete(NULL);
    }

    n = lwip_write(sd, buffer, n);
    if(n < 0 ){
        LogError(( "Error writing to socket (error %d)", (int)n ));
        LogError(( "Socket will be closed" ));
        lwip_close(sd);
        vTaskDelete(NULL);
    }

    lwip_close(sd);
    vTaskDelete(NULL);
}
//-----------------------------------------------------------------------------
//=============================================================================
