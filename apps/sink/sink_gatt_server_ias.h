/*******************************************************************************
Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sink_gatt_server_ias.h

DESCRIPTION
    Routines to handle messages sent from the GATT Immediate Alert Server Task.
    
NOTES

*/


#ifndef _SINK_GATT_SERVER_IMM_ALERT_H_
#define _SINK_GATT_SERVER_IMM_ALERT_H_


#ifndef GATT_ENABLED
#undef GATT_IAS_SERVER
#endif


/* Firmware headers */
#include <csrtypes.h>
#include <message.h>

/* Library headers */
#include <gatt_imm_alert_server.h>


#ifdef GATT_IAS_SERVER
#define sinkGattImmAlertServerGetSize() sizeof(GIASS_T)
#else
#define sinkGattImmAlertServerGetSize() 0
#endif

#ifdef GATT_IAS_SERVER
#define sinkGattServerImmAlertHigh(delay_s) MessageSendLater(&theSink.task, EventSysImmAlertHigh, 0, D_SEC(delay_s))
#define sinkGattServerImmAlertMild(delay_s) MessageSendLater(&theSink.task, EventSysImmAlertMild, 0, D_SEC(delay_s))
#else
#define sinkGattServerImmAlertHigh(delay_s) ((void)0)
#define sinkGattServerImmAlertMild(delay_s) ((void)0)
#endif

/*******************************************************************************
NAME
    sinkGattImmAlertServerInitialise
    
DESCRIPTION
    Initialise the Immediate Alert server task.
    
PARAMETERS
    None

RETURNS
    TRUE if the Immediate Alert server task was initialised, FALSE otherwise.
*/
#ifdef GATT_IAS_SERVER
bool sinkGattImmAlertServerInitialise(u16 **ptr);
#else
#define sinkGattImmAlertServerInitialise(ptr) (TRUE)
#endif

/*******************************************************************************
NAME
    sinkGattServerImmAlertStopAlert
    
DESCRIPTION
    Stop Alerting
    
PARAMETERS
    None
    
RETURNS
    void
*/
#ifdef GATT_IAS_SERVER
void sinkGattServerImmAlertStopAlert(void);
#else
#define sinkGattServerImmAlertStopAlert() ((void)(0))
#endif

/*******************************************************************************
NAME
    sinkGattImmAlertLocalAlert
    
DESCRIPTION
    Alert on headset/soundbar
    
PARAMETERS
    alert level
    
RETURNS
    void
*/
#ifdef GATT_IAS_SERVER
void sinkGattImmAlertLocalAlert(gatt_imm_alert_level alert_level);
#else
#define sinkGattImmAlertLocalAlert(alert_level) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkGattImmAlertServerMsgHandler
    
DESCRIPTION
    Handle messages from the GATT Immediate Alert Server library
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the GATT message
    payload The message payload
    
RETURNS
    void
*/
#ifdef GATT_IAS_SERVER
void sinkGattImmAlertServerMsgHandler(Task task, MessageId id, Message message);
#else
#define sinkGattImmAlertServerMsgHandler(task, id, message) ((void)(0))
#endif

#endif /* _SINK_GATT_SERVER_IMM_ALERT_H_ */
