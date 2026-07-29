
#ifndef TASKS_NETINIT_H_
#define TASKS_NETINIT_H_

//=============================================================================
/*-------------------------------- Includes ---------------------------------*/
//=============================================================================
#include <stdint.h>

//=============================================================================

//=============================================================================
/*--------------------------------- Defines ---------------------------------*/
//=============================================================================
#define NETINIT_CONFIG_TASK_PRIO        2
#define NETINIT_CONFIG_TASK_STACK_SIZE  2048

#ifndef NETINIT_CONFIG_USE_DHCP
#define NETINIT_CONFIG_USE_DHCP         1
#endif

#ifndef NETINIT_CONFIG_DEFAULT_IP
#define NETINIT_CONFIG_DEFAULT_IP       "192.168.0.91"
#endif

#ifndef NETINIT_CONFIG_DEFAULT_NETMASK
#define NETINIT_CONFIG_DEFAULT_NETMASK  "255.255.255.0"
#endif

#ifndef NETINIT_CONFIG_DEFAULT_GATEWAY
#define NETINIT_CONFIG_DEFAULT_GATEWAY  "192.168.0.1"
#endif

typedef struct{
    void (*on_init)(void);
    const uint8_t *mac;
}netinit_params_t;
//=============================================================================

//=============================================================================
/*---------------------------------- Task -----------------------------------*/
//=============================================================================
void netinit(void *param);
//=============================================================================

#endif /* TASKS_NETINIT_H_ */
