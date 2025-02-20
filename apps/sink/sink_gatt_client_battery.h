/*******************************************************************************
Copyright (c) 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0
*******************************************************************************/



#ifndef _SINK_GATT_CLIENT_BATTERY_H_
#define _SINK_GATT_CLIENT_BATTERY_H_


#ifndef GATT_ENABLED
#undef GATT_BATTERY_CLIENT
#endif


#include <gatt_battery_client.h>
#include "sink_powermanager.h"

#include <csrtypes.h>
#include <message.h>

/****************************************************************************
NAME    
    sinkGattBasClientSetupAdvertisingFilter
    
DESCRIPTION
    Adds Battery service to the advertising filter
*/    
#ifdef GATT_BATTERY_CLIENT
void gattBatteryClientSetupAdvertisingFilter(void);
#else
#define gattBatteryClientSetupAdvertisingFilter() ((void)(0))
#endif

/****************************************************************************
NAME    
    gattBatteryClientInit
    
DESCRIPTION
    Initialisation of battery client service
*/    
#ifdef GATT_BATTERY_CLIENT
void gattBatteryClientInit(void);
#else
#define gattBatteryClientInit() ((void)(0))
#endif

        
/****************************************************************************
NAME    
    gattBatteryClientAddService
    
DESCRIPTION
    Adds battery to list of client connection service.
    
PARAMETERS
    connection      The GATT client connection instance
    cid             The connection ID
    start           The start handle of the battery service
    end             The end handle of the battery service
    
RETURNS    
    TRUE if the battery service was successfully added, FALSE otherwise.
*/
#ifdef GATT_BATTERY_CLIENT
bool gattBatteryClientAddService(u16 cid, u16 start, u16 end);
#else
#define gattBatteryClientAddService(cid, start, end) (FALSE)
#endif


/****************************************************************************
NAME    
    gattBatteryClientRemoveService
    
DESCRIPTION
    Removes the battery service associated with the connection ID.
    
PARAMETERS
    gbasc           The battery client pointer
    cid             The connection ID
*/
#ifdef GATT_BATTERY_CLIENT
void gattBatteryClientRemoveService(GBASC *gbasc, u16 cid);
#else
#define gattBatteryClientRemoveService(gbasc, cid) ((void)(0))
#endif


/*******************************************************************************
NAME
    gattBatteryClientMsgHandler
    
DESCRIPTION
    Handle messages from the GATT Client Task library
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the GATT message
    payload The message payload
    
*/

#ifdef GATT_BATTERY_CLIENT
void gattBatteryClientMsgHandler(Task task, MessageId id, Message message);
#else
#define gattBatteryClientMsgHandler(task, id, message) ((void)(0))
#endif


/*******************************************************************************
NAME
    gattBatteryClientGetCachedLevel
    
DESCRIPTION
    Gets the cached battery level of a remote device when in client role.

*/
#ifdef GATT_BATTERY_CLIENT
u8 gattBatteryClientGetCachedLevel(void);
#else
#define gattBatteryClientGetCachedLevel() (BATTERY_LEVEL_INVALID)
#endif


/*******************************************************************************
NAME
    sinkGattBatteryClientEnabled
    
DESCRIPTION
    Returns if battery client is enabled.

*/
#ifdef GATT_BATTERY_CLIENT
#define sinkGattBatteryClientEnabled(void) (TRUE)
#else
#define sinkGattBatteryClientEnabled(void) (FALSE)
#endif


#endif /* _SINK_GATT_BATTERY_CLIENT_H_ */
