
//=============================================================================
/*-------------------------------- Includes ---------------------------------*/
//=============================================================================
#include "tcpclient.h"

/* Kernel */
#include "FreeRTOS.h"
#include "task.h"

/* lwip */
#include "netif/xadapter.h"
#include "lwip/sockets.h"

/* Logging config */
#include "clogging/logging_levels.h"

#ifndef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME    "TCP_CLIENT"
#endif

#ifndef LIBRARY_LOG_LEVEL
#define LIBRARY_LOG_LEVEL    LOG_DEBUG
#endif
#include "clogging/logging_stack.h"
//=============================================================================


//=============================================================================
/*---------------------------------- Task -----------------------------------*/
//=============================================================================
//-----------------------------------------------------------------------------
void tcpclient(void *params){

    char *server;
    unsigned int port;
    char *data;
    unsigned int data_size;
    unsigned int n_tx;

    int status;
    int sockfd;
    struct sockaddr_in servaddr;

    if( params != 0 ){
        tcpclient_params_t *p = (tcpclient_params_t *)params;
        server = p->server;
        port = p->port;
        data = p->data;
        data_size = p->data_size;
    }
    else{
        server = (char *)"127.0.0.1";
        port = 8081;
        data = 0;
        data_size = 0;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        LogError(("Failed to create socket"));
        vTaskDelete(NULL);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server);
    servaddr.sin_port = htons(port);

    status = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if( status != 0 ){
        LogError(("Failed to connect to server"));
        vTaskDelete(NULL);
    }

    LogInfo(("Connected to server. Sending message..."));

    n_tx = 0;
    while( n_tx < data_size ){
        status = send(sockfd, &data[n_tx], data_size - n_tx, 0);
        if( status <= 0 ){
            LogError(("Failed to write to server"));
            break;
        }
        n_tx += status;
    }

    LogInfo(("Closing client socket"));
    close(sockfd);
    vTaskDelete(NULL);
}
//-----------------------------------------------------------------------------
//=============================================================================
