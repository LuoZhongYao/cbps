/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0 

FILE NAME
    mapc_browsing.c

DESCRIPTION
    This file defines all APIs for Message Browsing feature supported 
    by the MAP Client library.
*/

#include "mapc.h"
#include "mapc_extern.h"

/**************************************************************************
 *NAME
 *  MapcMasSetFolderRequest
 *
 *DESCRIPTION
 *  The API function to navigate the folders of the MSE.
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 **************************************************************************/
void MapcMasSetFolderRequest( Mas masSession, 
                              bool  back,
                              const u8* name, 
                              u16 len )
{
    MAPC_ASSERT( masSession );
    if( !name ) len = 0;

    mapcPostMasSetFolder( masSession, back, name, len );
}

/*************************************************************************
 *NAME
 *  MapcMasGetFolderListingRequest
 *
 *DESCRIPTION
 *  The API function to get the folder listing
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasGetFolderListingRequest( Mas masSession, 
                                     u16 maxListCount, 
                                     u16 listStartOffset)
{

    MAPC_ASSERT( masSession );
    MAPC_ASSERT( maxListCount );
    mapcPostMasGetFolderListing( masSession, 
                                 maxListCount, 
                                 listStartOffset );
}

/*************************************************************************
 *NAME
 *  MapcMasGetMessagesListingRequest
 *
 *DESCRIPTION
 *  The API function to get the messages listing
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasGetMessagesListingRequest( Mas masSession, 
                                       const u8* name,
                                       u16 nameLen,
                                       u16 maxListCount, 
                                       u16 listStartOffset,
                                       MapcMessageFilter filter)
{
    MAPC_ASSERT( masSession );
    if( !name ) nameLen = 0;

    mapcPostMasGetMessagesListing( masSession, 
                                   name, 
                                   nameLen, 
                                   maxListCount, 
                                   listStartOffset,
                                   filter );
}

/*************************************************************************
 *NAME
 *  MapcMasPutMessage
 *
 *DESCRIPTION
 *  The API function to push message to the MSE
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasPutMessageRequest( Mas masSession, 
                               const u8* name,
                               u16 nameLen,
                               bool native,
                               bool moreData,
                               Source message )
{
    MAPC_ASSERT( masSession );
    if( !name ) nameLen = 0;

    mapcPostMasPutMessage(  masSession,
    					    name,
    					    nameLen,
    					    native,
                            moreData,
    					    message );

}

/*************************************************************************
 *NAME
 *  MapcMasPutContinue
 *
 *DESCRIPTION
 *  The API function to push message continue
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasPutContinue( Mas    masSession,
                         bool   moreData,
                         Source message )
{
    MAPC_ASSERT( masSession );

    mapcPostMasPutContinue( masSession,
                            moreData,
                            message );
}

/*************************************************************************
 *NAME
 *  MapcMasGetMessage
 *
 *DESCRIPTION
 *  The API function to get the message
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasGetMessageRequest( Mas masSession, 
                               const u8* handle,
                               bool native )
{
    MAPC_ASSERT( masSession );
    MAPC_ASSERT( handle );

    mapcPostMasGetMessage( masSession, handle, native );
}


/*************************************************************************
 *NAME
 *  MapcMasGetContinue
 *
 *DESCRIPTION
 *  The API function to continue the get operations.
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasGetContinue( Mas masSession )
{
    MAPC_ASSERT( masSession );
    
    mapcPostMasContinue( masSession, TRUE );
}

/*************************************************************************
 *NAME
 *  MapcMasGetComplete
 *
 *DESCRIPTION
 *  The API function to complete the get operations.
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasGetComplete( Mas masSession )
{
    MAPC_ASSERT( masSession );
    
    mapcPostMasContinue( masSession, FALSE );
}

/*************************************************************************
 *NAME
 *  MapcMasUpdateInboxRequest
 *
 *DESCRIPTION
 *  The API function to update inbox
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void MapcMasUpdateInboxRequest( Mas masSession )
{
    MAPC_ASSERT( masSession );
    
    mapcPostMasUpdateInbox( masSession );
}

/*************************************************************************
 *NAME
 *  MapcMasSetMessageStatus
 *
 *DESCRIPTION
 *  The API function to set the message status
 *
 *PARAMETERS
 *  Refer mapc.h for details 
 *************************************************************************/
void  MapcMasSetMessageStatusRequest( Mas masSession,
                                      const u8* handle,
                                      MapcMessageStatus status )
{
    u8 indicator = 0, value = 0;

    MAPC_ASSERT( masSession );

    if( status == mapc_message_deleted || status == mapc_message_undeleted )
    {
        indicator = 1;
    }

    if( status == mapc_message_deleted || status == mapc_message_read )
    {
        value = 1;
    }

    mapcPostMasSetMessageStatus( masSession, handle, indicator, value );
}

