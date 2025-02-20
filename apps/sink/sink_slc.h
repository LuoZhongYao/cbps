/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_slc.h
    
DESCRIPTION
    
*/

#ifndef _SINK_SLC_H_
#define _SINK_SLC_H_

#include "sink_devicemanager.h"

typedef enum 
{
    profile_type_hfp,
    profile_type_a2dp,
    profile_type_both
}profile_type;

/*the action to take on the auto reconnect*/
typedef enum AutoReconnectActionTag
{
    AR_LastConnected    = 0 ,   
    AR_List             = 1 ,
    AR_Rssi             = 2 
}ARAction_t ;


#define INVALID_LIST_ID 0xFF
#define MAX_SLC_CONNECTIONS 2

#define for_all_hfp(idx)    for((idx) = hfp_primary_link; (idx) <= hfp_secondary_link; (idx)++)



/****************************************************************************
NAME    
    slcConnectFail
    
DESCRIPTION
    SLC failed to connect
RETURNS
    void
*/
void slcConnectFail(void);


/****************************************************************************
NAME    
    sinkHandleSlcConnectInd
    
DESCRIPTION
    Handle a request to establish an SLC from the AG.

RETURNS
    void
*/
void sinkHandleSlcConnectInd( const HFP_SLC_CONNECT_IND_T *ind );


/****************************************************************************
NAME    
    sinkHandleSlcDisconnectInd
    
DESCRIPTION
    Indication that the SLC has been released.

RETURNS
    void
*/
void sinkHandleSlcDisconnectInd( const HFP_SLC_DISCONNECT_IND_T *ind );


/****************************************************************************
NAME    
    slcEstablishSLCRequest
    
DESCRIPTION
    Request to create a connection to a remote AG.

RETURNS
    void
*/

void slcEstablishSLCRequest ( void );


/****************************************************************************
NAME    
    slcContinueEstablishSLCRequest
    
DESCRIPTION
    continue the connection request to create a connection to a remote AG.

RETURNS
    void
*/

void slcContinueEstablishSLCRequest (void);


/****************************************************************************
NAME    
    slcConnectDevice
    
DESCRIPTION
    Attempt to connect profiles (as defined in sink_devicemanager.h) to a 
    given device 

RETURNS
    void
*/
void slcConnectDevice(bdaddr* bd_addr, sink_link_type profiles);


/****************************************************************************
NAME    
    slcAttemptConnection
    
DESCRIPTION
    attemp connection to next item in pdl

RETURNS
    void
*/
void slcAttemptConnection(void);


/****************************************************************************
NAME    
    slcDetermineConnectAction
    
DESCRIPTION
    Request to determine the connection action required, 

RETURNS
    required action based on current device state
*/

ARAction_t slcDetermineConnectAction( void );


/****************************************************************************
NAME    
    sinkHandleSlcConnectCfm
    
DESCRIPTION
    Confirmation that the SLC has been established (or not).

RETURNS
    void
*/
bool sinkHandleSlcConnectCfm( const HFP_SLC_CONNECT_CFM_T *cfm );


/****************************************************************************
NAME    
    slcHandleLinkLossInd
    
DESCRIPTION
    Indication of change in link loss status.

RETURNS
    void
*/
void slcHandleLinkLossInd( const HFP_SLC_LINK_LOSS_IND_T *ind );


/****************************************************************************
NAME    
    sinkDisconnectAllSlc
    
DESCRIPTION
    Disconnect all the SLC's 

RETURNS
    void
*/
void sinkDisconnectAllSlc( void );

/****************************************************************************
NAME    
    sinkDisconnectSlcFromDevice
    
DESCRIPTION
    Disconnect SLC with the device provided.

RETURNS
    void
*/
void sinkDisconnectSlcFromDevice(const bdaddr *bdaddr_non_gaia_device);

/****************************************************************************
NAME    
    slcGetNextAvBdAddress
    
DESCRIPTION
    Dtermines the BD Address of the next connected device.

RETURNS
    void
*/
bool slcGetNextAvBdAddress(const bdaddr *bd_addr , bdaddr *next_bdaddr );

/****************************************************************************
NAME    
    slcGetLinkFromBdAddress
    
DESCRIPTION
    Dtermines the BD Address of the device connected with the given link priority,

RETURNS
    void
*/
bool slcGetLinkFromBdAddress(const bdaddr *bd_addr , hfp_link_priority *link_priority);

/****************************************************************************
NAME    
    slcReset
    
DESCRIPTION
    reset the pdl connection pointer

RETURNS
    none
*/
void slcReset(void);

/****************************************************************************
NAME    
    isTWSDeviceAvailable
    
DESCRIPTION
    looks to see if passed in pdl index is a TWS device

RETURNS
    TRUE or FALSE
*/
#ifdef ENABLE_PEER
bool isTWSDeviceAvailable(u8 Id);
#endif
/****************************************************************************
NAME    
    isPdlEntryAvailable
    
DESCRIPTION
    looks to see if passed in pdl index is already connected 

RETURNS
    TRUE or FALSE
*/
bool isPdlEntryAvailable( u8 Id );


/****************************************************************************
NAME    
    slcGetNextListID
    
DESCRIPTION
    selects the next available ListID for connection based on list id passed in and
    the type of profile requested, this could be 'not fussed', 'hfp' or 'a2dp', the 
    funtion will also check for the end of the pdl and wrap to the beggining if that 
    feature is enabled

RETURNS
    true or false success status
*/   
bool slcGetNextListID(void);


/****************************************************************************
NAME    
    slcIsListIdAvailable
    
DESCRIPTION
    determine whether the ListID passed in is available in the PDL, the ID
    passed in could be out of range of the PDL so checks for that also

RETURNS
    true or false success status
*/   
bool slcIsListIdAvailable(u8 ListID);

#endif /* _SINK_SLC_H_ */

