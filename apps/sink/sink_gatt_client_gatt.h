

#ifndef _SINK_GATT_CLIENT_GATT_H_
#define _SINK_GATT_CLIENT_GATT_H_


#include <gatt_client.h>

#include <csrtypes.h>
#include <message.h>

        
/****************************************************************************
NAME    
    sinkGattClientServiceAdd
    
DESCRIPTION
    Adds GATT to list of client connection service.
    
PARAMETERS
    cid             The connection ID
    start           The start handle of the GATT service
    end             The end handle of the GATT service
    
RETURNS    
    TRUE if the GATT service was successfully added, FALSE otherwise.
*/
#ifdef GATT_ENABLED
bool sinkGattClientServiceAdd(u16 cid, u16 start, u16 end);
#else
#define sinkGattClientServiceAdd(cid, start, end) (FALSE)
#endif


/****************************************************************************
NAME    
    sinkGattClientServiceRemove
    
DESCRIPTION
    Removes the GATT service associated with the connection ID.
    
PARAMETERS
    gbasc           The GATT client pointer
    cid             The connection ID
*/
#ifdef GATT_ENABLED
void sinkGattClientServiceRemove(GGATTC *ggattc, u16 cid);
#else
#define sinkGattClientServiceRemove(ggattc, cid) ((void)(0))
#endif


/*******************************************************************************
NAME
    sinkGattClientServiceMsgHandler
    
DESCRIPTION
    Handle messages from the GATT Client Task library
    
PARAMETERS
    task    The task the message is delivered
    id      The ID for the GATT message
    payload The message payload
    
*/

#ifdef GATT_ENABLED
void sinkGattClientServiceMsgHandler(Task task, MessageId id, Message message);
#else
#define sinkGattClientServiceMsgHandler(task, id, message) ((void)(0))
#endif


#endif /* _SINK_GATT_CLIENT_GATT_H_ */
