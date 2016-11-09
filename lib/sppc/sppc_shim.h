/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sppc_shim.h
    
DESCRIPTION
	SPPC Server Shim Layer
	
*/

#ifndef SPPC_SHIM_LAYER_H
#define SPPC_SHIM_LAYER_H

#include <sppc.h>

typedef struct __SPP SPPC;

#define SPPC_SHIM_MESSAGE_BASE SPP_MESSAGE_TOP
typedef enum
{
    SPPC_MESSAGE_MORE_DATA_TEST_EXTRA = SPPC_SHIM_MESSAGE_BASE,
    SPPC_CLIENT_CONNECT_CFM_TEST_EXTRA,
    SPPC_DISCONNECT_IND_TEST_EXTRA,
    SPPC_DISCONNECT_CFM_TEST_EXTRA,

    /* Library message limit */    
    SPPC_SHIM_MESSAGE_TOP
} SppcShimMessageId;


typedef struct
{
    Sink    sink;
    
    u16  size_data;
    u8   data[1];
} SPPC_MESSAGE_MORE_DATA_TEST_EXTRA_T;

typedef struct 
{
    SPP                         *spp;
    spp_connect_status          status;
    Sink                        sink;
    u16                      payload_size;
} SPPC_CLIENT_CONNECT_CFM_TEST_EXTRA_T;

typedef struct 
{
    SPP                     *spp;
    spp_disconnect_status   status; 
    Sink                    sink;
} SPPC_DISCONNECT_IND_TEST_EXTRA_T;

typedef struct 
{
    SPP                     *spp;
    spp_disconnect_status   status; 
    Sink                    sink;
} SPPC_DISCONNECT_CFM_TEST_EXTRA_T;



/*****************************************************************************/
void SppcHandleComplexMessage(Task task, MessageId id, Message message);

/*****************************************************************************/
void SppcConnectRequestTestExtra(Task theAppTask, const bdaddr *bd_addr, const u16 security_channel, u16 max_payload_size);

/*****************************************************************************/
void SppcDisconnectResponseTestExtra(SPPC *sppc);

/*****************************************************************************/
void SppcDisconnectRequestTestExtra(SPPC *sppc);


#endif

