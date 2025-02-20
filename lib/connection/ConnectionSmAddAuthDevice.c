/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    ConnectionSmAddAuthDevice.c        

DESCRIPTION
    This file contains the management entity responsible for device security

NOTES

*/


/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"

#include <message.h>
#include <string.h>
#include <vm.h>

/*****************************************************************************/
void ConnectionSmAddAuthDevice(Task theAppTask, const bdaddr *peer_bd_addr, u16 trusted, u16 bonded, u8 key_type, u16 size_link_key, const u16* link_key)
{
#ifdef CONNECTION_DEBUG_LIB
    if (size_link_key != BREDR_KEY_SIZE)
    {
        CL_DEBUG(("Link key size %d not supported\n", size_link_key))
    }
#endif

    {
        MAKE_CL_MESSAGE(CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ);
        message->theAppTask = theAppTask;
        message->bd_addr = *peer_bd_addr;

        message->enc_bredr.link_key_type = 
            connectionConvertLinkKeyType_t(key_type);
        memmove(message->enc_bredr.link_key, link_key, BREDR_KEY_SIZE);
        
        message->trusted = trusted;
        message->bonded = bonded;
        MessageSend(connectionGetCmTask(), CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ, message);
    }
}
