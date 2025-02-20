/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    ConnectionL2capUnregisterRequest.c        

DESCRIPTION
    File containing the l2cap API function implementations.    

NOTES

*/


/****************************************************************************
    Header files
*/
#include    "connection.h"
#include    "connection_private.h"

#ifndef CL_EXCLUDE_L2CAP

/*****************************************************************************/
void ConnectionL2capUnregisterRequest(Task appTask, u16 psm)
{
    /* Send an internal message */
    MAKE_CL_MESSAGE(CL_INTERNAL_L2CAP_UNREGISTER_REQ);
    message->theAppTask = appTask;
    message->app_psm = psm;
    MessageSend(connectionGetCmTask(), CL_INTERNAL_L2CAP_UNREGISTER_REQ, message);
}

#endif

