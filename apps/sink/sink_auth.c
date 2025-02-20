/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_auth.c

DESCRIPTION
    This file contains the Authentication functionality for the Sink
    Application

NOTES

*/

/****************************************************************************
    Header files
*/
#include "sink_private.h"
#include "sink_audio_prompts.h"
#include "sink_statemanager.h"
#include "sink_auth.h"

#include "sink_devicemanager.h"
#include "sink_debug.h"
#include "sink_ble_gap.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#include <ps.h>
#include <bdaddr.h>
#include <stdlib.h>
#include <sink.h>

#ifdef DEBUG_AUTH
#else
#endif

/* In case of RRA enabled devices  the key distribution is as follows
    when we are initiator we require the responder to distribute its LTK, EDIV, RAND and IRK
    when we are responder  we require the initiator to distribute its LTK, EDIV, RAND and IRK.
*/
#define KEY_DISTRIBUTION_RANDOM (KEY_DIST_RESPONDER_ENC_CENTRAL | KEY_DIST_RESPONDER_ID | KEY_DIST_INITIATOR_ENC_CENTRAL | KEY_DIST_INITIATOR_ID)
#define KEY_DISTRIBUTION_PUBLIC  (KEY_DIST_RESPONDER_ENC_CENTRAL | KEY_DIST_INITIATOR_ENC_CENTRAL)

#ifdef ENABLE_PEER
#define BD_ADDR_SIZE (sizeof(bdaddr))
#define LINK_KEY_SIZE 8
#define ATTRIBUTES_SIZE (sizeof(sink_attributes))
#define STATUS_LOC 0
#define BD_ADDR_LOC 1
#define LINK_KEY_LOC (BD_ADDR_LOC+BD_ADDR_SIZE)
#define ATTRIBUTES_LOC (LINK_KEY_LOC+LINK_KEY_SIZE)

static void readPsPermanentPairing (bdaddr *bd_addr, u16 *link_key, u16 *link_key_status, sink_attributes *attributes)
{
    u16 * ps_key;

    /* Allocate and zero buffer to hold PS key */
    ps_key = mallocPanic(BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);
    memset(ps_key, 0, BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);

    /* Attempt to obtain current pairing data */
    PsRetrieve(CONFIG_PERMANENT_PAIRING, ps_key, BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);

    /* Return any requested fields */
    if (link_key_status)
    {
        *link_key_status = ps_key[STATUS_LOC];
    }

    if (bd_addr)
    {
        memcpy(bd_addr, &ps_key[BD_ADDR_LOC], BD_ADDR_SIZE);
    }

    if (link_key)
    {
        memcpy(link_key, &ps_key[LINK_KEY_LOC], LINK_KEY_SIZE);
    }

    if (attributes)
    {
        memcpy(attributes, &ps_key[ATTRIBUTES_LOC], ATTRIBUTES_SIZE);
    }

#ifdef DEBUG_DEV
    {
        bdaddr *perm_addr = (bdaddr *) &ps_key[BD_ADDR_LOC];
        sink_attributes *perm_attributes = (sink_attributes *) &ps_key[ATTRIBUTES_LOC];

        DEBUG(("DEV: perm read %04X %02X %06lX prof:0x%02X route:%u,%u\n",
               perm_addr->nap,
               perm_addr->uap,
               perm_addr->lap,
               perm_attributes->profiles,
               perm_attributes->master_routing_mode,
               perm_attributes->slave_routing_mode));
    }
#endif

    free(ps_key);
}

static void writePsPermanentPairing (const bdaddr *bd_addr, u16 *link_key, u16 link_key_status, const sink_attributes *attributes)
{
    u16 * ps_key;

    /* Allocate and zero buffer to hold PS key */
    ps_key = mallocPanic(BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);
    memset(ps_key, 0, BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);

    /* Attempt to obtain current pairing data */
    PsRetrieve(CONFIG_PERMANENT_PAIRING, ps_key, BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);

    /* Update supplied fields */
    if (link_key_status)
    {
        ps_key[STATUS_LOC] = link_key_status;
    }

    if (bd_addr)
    {
        memcpy(&ps_key[BD_ADDR_LOC], bd_addr, BD_ADDR_SIZE);
    }

    if (link_key)
    {
        memcpy(&ps_key[LINK_KEY_LOC], link_key, LINK_KEY_SIZE);
    }

    if (attributes)
    {
        memcpy(&ps_key[ATTRIBUTES_LOC], attributes, ATTRIBUTES_SIZE);
    }

#ifdef DEBUG_DEV
    {
        bdaddr *perm_addr = (bdaddr *) &ps_key[BD_ADDR_LOC];
        sink_attributes *perm_attributes = (sink_attributes *) &ps_key[ATTRIBUTES_LOC];

        DEBUG(("DEV: perm write %04X %02X %06lX prof:0x%02X route:%u,%u\n",
               perm_addr->nap,
               perm_addr->uap,
               perm_addr->lap,
               perm_attributes->profiles,
               perm_attributes->master_routing_mode,
               perm_attributes->slave_routing_mode));
    }
#endif

    /* Store updated pairing data */
    PsStore(CONFIG_PERMANENT_PAIRING, ps_key, BD_ADDR_SIZE + LINK_KEY_SIZE + ATTRIBUTES_SIZE + 1);

    free(ps_key);
}

/****************************************************************************
NAME
    AuthInitPermanentPairing

DESCRIPTION
    Add devices stored in CONFIG_PERMANENT_PAIRING to Connection library's PDL

RETURNS
    void
*/
void AuthInitPermanentPairing (void)
{
    bdaddr ps_bd_addr;

    LOGD("AuthInitPermanentPairing\n");

    /* Obtain just the bluetooth address of a permanently paired device */
    readPsPermanentPairing(&ps_bd_addr, 0, 0, 0);
    LOGD("   ps bd_addr = [%x:%x:%lx]\n", ps_bd_addr.uap, ps_bd_addr.nap, ps_bd_addr.lap);

    if ( !BdaddrIsZero(&ps_bd_addr) )
    {   /* A valid address has been obtained, ask CL for info on it */
        ConnectionSmGetAuthDevice(&theSink.task, (const bdaddr *)&ps_bd_addr);
    }
}

/****************************************************************************
NAME
    AuthRemovePermanentPairing

DESCRIPTION
    Removes permanent paired device from Connection library's PDL.
    Will also erase CONFIG_PERMANENT_PAIRING if erase_ps_key set TRUE.

RETURNS
    void
*/
void AuthRemovePermanentPairing (bool erase_ps_key)
{
    bdaddr ps_bd_addr;

    LOGD("AuthRemovePermanentPairing  erase_ps_key = %u\n", erase_ps_key);

    readPsPermanentPairing(&ps_bd_addr, 0, 0, 0);
    LOGD("   ps bd_addr = [%x:%x:%lx]\n", ps_bd_addr.uap, ps_bd_addr.nap, ps_bd_addr.lap);

    if ( !BdaddrIsZero(&ps_bd_addr) )
    {
        ConnectionSmDeleteAuthDeviceReq(TYPED_BDADDR_PUBLIC, (const bdaddr *)&ps_bd_addr);
    }

    if ( erase_ps_key )
    {
        PsStore(CONFIG_PERMANENT_PAIRING, 0, 0);
    }
}

/****************************************************************************
NAME
    AuthUpdatePermanentPairing

DESCRIPTION
    Use supplied BDADDR to obtain linkkey from Connection library and update
    CONFIG_PERMANENT_PAIRING to retain this as the permanently paired device

RETURNS
    void
*/
void AuthUpdatePermanentPairing (const bdaddr *bd_addr, const sink_attributes *attributes)
{
    bdaddr ps_bdaddr;
    LOGD("AuthUpdatePermanentPairing\n");

    readPsPermanentPairing(&ps_bdaddr, 0, 0, 0);

    if(!BdaddrIsZero(&ps_bdaddr) && !BdaddrIsSame(&ps_bdaddr, bd_addr))
    {
        AuthRemovePermanentPairing(FALSE);
    }

    /* Update permanent pairing info */
    writePsPermanentPairing(0, 0, 0, attributes);

    ConnectionSmGetAuthDevice(&theSink.task, bd_addr);
}

/****************************************************************************
NAME
    handleGetAuthDeviceCfm

DESCRIPTION
    Called in response to CL_SM_GET_AUTH_DEVICE_CFM message, which is generated
    due to calling updatePermanentPairing.
    Both the BDADDR and linkkey contained in CL_SM_GET_AUTH_DEVICE_CFM are used to
    update CONFIG_PERMANENT_PAIRING to retain this as the permanently paired device

RETURNS
    void
*/
void handleGetAuthDeviceCfm (CL_SM_GET_AUTH_DEVICE_CFM_T *cfm)
{
    LOGD("handleGetAuthDeviceCfm\n");
    LOGD("   status = %u\n",cfm->status);
    LOGD("   ps bd_addr = [%x:%x:%lx]\n", cfm->bd_addr.uap, cfm->bd_addr.nap, cfm->bd_addr.lap);
    LOGD("   trusted = %u\n",cfm->trusted);
    LOGD("   link key type = %u",cfm->link_key_type);
    LOGD("   link key size = %u\n",cfm->size_link_key);

    if ( cfm->status == success )
    {   /* Device exists in CL PDL */
        sink_attributes attributes;
        u16 link_key_status = ((cfm->trusted & 0xF)<<8) | ((cfm->link_key_type & 0xF)<<4) | (cfm->size_link_key & 0xF);

        if (!deviceManagerGetAttributes(&attributes, &cfm->bd_addr))
        {
            bdaddr perm_bdaddr;

            /* No attributes in PDL, so check if attributes for this bdaddr
               are stored in the permanent pairing data.
               If not, revert to defaults. */
            readPsPermanentPairing(&perm_bdaddr, 0, 0, &attributes);
            if (BdaddrIsZero(&perm_bdaddr) || !BdaddrIsSame(&perm_bdaddr, &cfm->bd_addr))
            {
                deviceManagerGetDefaultAttributes(&attributes, dev_type_none);
            }
        }

        /* Update permanent pairing info */
        writePsPermanentPairing(&cfm->bd_addr, cfm->link_key, link_key_status, &attributes);

        /* Update attributes */
        deviceManagerStoreAttributes(&attributes, (const bdaddr *)&cfm->bd_addr);

        /* Mark the device as trusted and push it to the top of the PDL */
        ConnectionSmUpdateMruDevice((const bdaddr *)&cfm->bd_addr);

        deviceManagerUpdatePriorityDevices();
    }
    else
    {   /* Device *does not* exist in CL PDL */
        bdaddr ps_bd_addr;
        u16 ps_link_key_status;
        u16 ps_link_key[LINK_KEY_SIZE];

        readPsPermanentPairing(&ps_bd_addr, ps_link_key, &ps_link_key_status, 0);

        if ( !BdaddrIsZero(&ps_bd_addr) )
        {   /* We have permanently paired device, add it to CL PDL */
            bool trusted = (bool)((ps_link_key_status>>8) & 0xF);
            cl_sm_link_key_type key_type = (cl_sm_link_key_type)((ps_link_key_status>>4) & 0xF);
            u16 size_link_key = ps_link_key_status & 0xF;

            ConnectionSmAddAuthDevice(&theSink.task, (const bdaddr *)&ps_bd_addr, trusted, TRUE, key_type, size_link_key, (const u16 *)ps_link_key);
        }
    }
 }

/****************************************************************************
NAME
    handleAddAuthDeviceCfm

DESCRIPTION
    Called in response to CL_SM_ADD_AUTH_DEVICE_CFM message, which is generated
    due to calling ConnectionSmAddAuthDevice.

RETURNS
    void
*/
void handleAddAuthDeviceCfm (CL_SM_ADD_AUTH_DEVICE_CFM_T *cfm)
{
    if ( cfm->status == success )
    {   /* Ask for device info again to allow write of attribute data */
        ConnectionSmGetAuthDevice(&theSink.task, (const bdaddr *)&cfm->bd_addr);
    }
}
#endif  /* ENABLE_PEER */

/****************************************************************************
NAME
    AuthCanSinkConnect

DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanSinkConnect ( const bdaddr * bd_addr );

/****************************************************************************
NAME
    AuthCanSinkPair

DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanSinkPair ( void ) ;

/*************************************************************************
NAME
     sinkHandlePinCodeInd

DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The Sink devices default pin code is sent back.

RETURNS

*/
void sinkHandlePinCodeInd(const CL_SM_PIN_CODE_IND_T* ind)
{
    u16 pin_length = 0;
    u8 pin[16];

    if ( AuthCanSinkPair() )
    {

		LOGD("auth: Can Pin\n");

   		/* Do we have a fixed pin in PS, if not reject pairing */
    	if ((pin_length = PsFullRetrieve(PSKEY_FIXED_PIN, pin, 16)) == 0 || pin_length > 16)
   		{
   	    	/* Set length to 0 indicating we're rejecting the PIN request */
        	LOGD("auth : failed to get pin\n");
       		pin_length = 0;
   		}
        else if(theSink.features.VoicePromptPairing)
        {
            AudioPromptPlayEvent(EventSysPinCodeRequest);
            AudioPromptPlayNumString(pin_length, pin);
        }
	}
    /* Respond to the PIN code request */
    ConnectionSmPinCodeResponse(&ind->taddr, pin_length, pin);
}

/*************************************************************************
NAME
     sinkHandleUserConfirmationInd

DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS

*/
void sinkHandleUserConfirmationInd(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind)
{
	/* Can we pair? */
	if ( AuthCanSinkPair() && theSink.features.ManInTheMiddle)
    {
        theSink.confirmation = TRUE;
		LOGD("auth: Can Confirm %ld\n",ind->numeric_value);
		/* Should use text to speech here */
		theSink.confirmation_addr  = mallocPanic(sizeof(tp_bdaddr));
		*theSink.confirmation_addr = ind->tpaddr;
        if(theSink.features.VoicePromptPairing)
        {
            AudioPromptPlayEvent(EventSysConfirmationRequest);
            AudioPromptPlayNumber(ind->numeric_value);
        }
	}
	else
    {
		/* Reject the Confirmation request */
		LOGD("auth: Rejected Confirmation Req\n");
		ConnectionSmUserConfirmationResponse(&ind->tpaddr, FALSE);
    }
}

/*************************************************************************
NAME
     sinkHandleUserPasskeyInd

DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_IND

RETURNS

*/
void sinkHandleUserPasskeyInd(const CL_SM_USER_PASSKEY_REQ_IND_T* ind)
{
	/* Reject the Passkey request */
	LOGD("auth: Rejected Passkey Req\n");
	ConnectionSmUserPasskeyResponse(&ind->tpaddr, TRUE, 0);
}


/*************************************************************************
NAME
     sinkHandleUserPasskeyNotificationInd

DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS

*/
void sinkHandleUserPasskeyNotificationInd(const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind)
{
	LOGD("Passkey: %ld \n", ind->passkey);
    if(theSink.features.ManInTheMiddle && theSink.features.VoicePromptPairing)
    {
        AudioPromptPlayEvent(EventSysPasskeyDisplay);
        AudioPromptPlayNumber(ind->passkey);
    }
	/* Should use text to speech here */
}

/*************************************************************************
NAME
     sinkHandleIoCapabilityInd

DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS

*/
void sinkHandleIoCapabilityInd(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind)
{
    u16 key_dist = KEY_DIST_NONE;
    bool can_pair = FALSE;
    const tp_bdaddr* remote_bdaddr = &ind->tpaddr;

    /* Check if this is for a BR/EDR device */
    if((remote_bdaddr->transport == TRANSPORT_BREDR_ACL) && (AuthCanSinkPair()))
    {
        can_pair = TRUE;
    }

    /* Check if this is for a LE device */
    if ((remote_bdaddr->transport == TRANSPORT_BLE_ACL) && (sinkBleGapIsBondable()))
    {
        can_pair = TRUE;
        key_dist = (remote_bdaddr->taddr.type == TYPED_BDADDR_RANDOM) ?
                        KEY_DISTRIBUTION_RANDOM : KEY_DISTRIBUTION_PUBLIC;
    }

    /* If not pairable should reject */
    if(can_pair)
    {
        cl_sm_io_capability local_io_capability = theSink.features.ManInTheMiddle ? cl_sm_io_cap_display_yes_no : cl_sm_io_cap_no_input_no_output;
        mitm_setting sink_mitm_setting = theSink.features.ManInTheMiddle ? mitm_required : mitm_not_required;

        LOGD("auth: Sending IO Capability \n");

        /* Send Response and request to bond with device */
        ConnectionSmIoCapabilityResponse(&ind->tpaddr, local_io_capability, sink_mitm_setting, TRUE, key_dist, oob_data_none, NULL, NULL);
    }
    else
    {
        LOGD("auth: Rejecting IO Capability Req \n");
        ConnectionSmIoCapabilityResponse(&ind->tpaddr, cl_sm_reject_request, mitm_not_required, FALSE, key_dist, oob_data_none, NULL, NULL);
    }
}

/*************************************************************************
NAME
     sinkHandleRemoteIoCapabilityInd

DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND

RETURNS

*/
void sinkHandleRemoteIoCapabilityInd(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind)
{
	LOGD("auth: Incoming Authentication Request\n");
}

/****************************************************************************
NAME
    sinkHandleAuthoriseInd

DESCRIPTION
    Request to authorise access to a particular service.

RETURNS
    void
*/
void sinkHandleAuthoriseInd(const CL_SM_AUTHORISE_IND_T *ind)
{

	bool lAuthorised = FALSE ;

	if ( AuthCanSinkConnect(&ind->bd_addr) )
	{
		lAuthorised = TRUE ;
	}

	LOGD("auth: Authorised [%d]\n" , lAuthorised);

	/*complete the authentication with the authorised or not flag*/
    ConnectionSmAuthoriseResponse(&ind->bd_addr, ind->protocol_id, ind->channel, ind->incoming, lAuthorised);
}


/****************************************************************************
NAME
    sinkHandleAuthenticateCfm

DESCRIPTION
    Indicates whether the authentication succeeded or not.

RETURNS
    void
*/
void sinkHandleAuthenticateCfm(const CL_SM_AUTHENTICATE_CFM_T *cfm)
{
#ifdef ENABLE_SUBWOOFER
    if (theSink.inquiry.action == rssi_subwoofer)
    {
        if ((cfm->status == auth_status_success) && (cfm->bonded))
        {
            /* Mark the subwoofer as a trusted device */
            deviceManagerMarkTrusted(&cfm->bd_addr);

            /* Store the subwoofers BDADDR to PS */
            configManagerWriteSubwooferBdaddr(&cfm->bd_addr);

            /* Setup some default attributes for the subwoofer */
            deviceManagerStoreDefaultAttributes(&cfm->bd_addr, dev_type_sub);

            /* mark the subwoofer device as DO NOT DELETE in PDL */
            ConnectionAuthSetPriorityDevice((const bdaddr *)&cfm->bd_addr, TRUE);
        }
        return;
    }
#endif
	/* Leave bondable mode if successful unless we got a debug key */
	if (cfm->status == auth_status_success && cfm->key_type != cl_sm_link_key_debug)
    {
        if ((theSink.inquiry.action != rssi_pairing) || (theSink.inquiry.session != inquiry_session_normal))
        {
            /* Mark the device as trusted */
            deviceManagerMarkTrusted(&cfm->bd_addr);
            MessageSend (&theSink.task , EventSysPairingSuccessful , 0 );
        }
    }

	/* Set up some default params and shuffle PDL */
	if(cfm->bonded)
	{
        sink_attributes attributes;

        deviceManagerClearAttributes(&attributes);
        if(!deviceManagerGetAttributes(&attributes, &cfm->bd_addr))
        {
            deviceManagerStoreDefaultAttributes(&cfm->bd_addr, dev_type_ag);
        }
        else
        {
            deviceManagerUpdateAttributesWithDeviceDefaults(&attributes, dev_type_ag);
            deviceManagerStoreAttributes(&attributes, &cfm->bd_addr);
        }

        ConnectionAuthSetPriorityDevice((const bdaddr *)&cfm->bd_addr, FALSE);
	}

	/* Reset pairing info if we timed out on confirmation */
	AuthResetConfirmationFlags();
}


/****************************************************************************
NAME
    AuthCanSinkPair

DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanSinkPair ( void )
{
	bool lCanPair = FALSE ;

    if (theSink.features.SecurePairing)
    {
	    	/*if we are in pairing mode*/
		if ((stateManagerGetState() == deviceConnDiscoverable)||(theSink.inquiry.action == rssi_subwoofer))
		{
			lCanPair = TRUE ;
			LOGD("auth: is ConnDisco\n");
		}
#ifdef ENABLE_PARTYMODE
        else if(theSink.PartyModeEnabled)
        {
			lCanPair = TRUE ;
			LOGD("auth: allow PartyMode pairing\n");
        }
#endif
    }
    else
    {
	    lCanPair = TRUE ;
    }

    return lCanPair ;
}



/****************************************************************************
NAME
    AuthCanSinkConnect

DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanSinkConnect ( const bdaddr * bd_addr )
{
	bool lCanConnect = FALSE ;
    u8 NoOfDevices = deviceManagerNumConnectedDevs();

    /* if device is already connected via a different profile allow this next profile to connect */
    if(deviceManagerProfilesConnected(bd_addr))
    {
    	LOGD("auth: already connected, CanConnect = TRUE\n");
        lCanConnect = TRUE;
    }
    /* this bdaddr is not already connected, therefore this is a new device, ensure it is allowed
       to connect, if not reject it */
    else
    {
        /* when multipoint is turned off, only allow one device to connect */
        if(((!theSink.MultipointEnable)&&(!NoOfDevices))||
           ((theSink.MultipointEnable)&&(NoOfDevices < MAX_MULTIPOINT_CONNECTIONS)))
        {
            /* is secure pairing enabled? */
            if (theSink.features.SecurePairing)
            {
    	        /* If page scan is enabled (i.e. we are either connectable/discoverable or
    	    	connected in multi point) */
    	    	if ( theSink.page_scan_enabled )
    	    	{
    	    		lCanConnect = TRUE ;
    	    		LOGD("auth: is connectable\n");
    	    	}
            }
            /* no secure pairing */
            else
            {
            	LOGD("auth: MP CanConnect = TRUE\n");
    	        lCanConnect = TRUE ;
            }
        }
    }

    LOGD("auth:  CanConnect = %d\n",lCanConnect);

    return lCanConnect ;
}

/****************************************************************************
NAME
    sinkPairingAcceptRes

DESCRIPTION
    Respond correctly to a pairing info request ind

RETURNS
    void
*/
void sinkPairingAcceptRes( void )
{
    if(AuthCanSinkPair() && theSink.confirmation)
	{
		LOGD("auth: Accepted Confirmation Req\n");
		ConnectionSmUserConfirmationResponse(theSink.confirmation_addr, TRUE);
     }
	else
     {
		LOGD("auth: Invalid state for confirmation\n");
     }
}

/****************************************************************************
NAME
    sinkPairingRejectRes

DESCRIPTION
    Respond reject to a pairing info request ind

RETURNS
    void
*/
void sinkPairingRejectRes( void )
{
	if(AuthCanSinkPair() && theSink.confirmation)
	{
		LOGD("auth: Rejected Confirmation Req\n");
		ConnectionSmUserConfirmationResponse(theSink.confirmation_addr, FALSE);
	}
	else
	{
		LOGD("auth: Invalid state for confirmation\n");
	}
}

/****************************************************************************
NAME
    AuthResetConfirmationFlags

DESCRIPTION
    Helper function to reset the confirmations flag and associated BT address

RETURNS

*/

void AuthResetConfirmationFlags ( void )
{
	LOGD("auth: Reset Confirmation Flags\n");
	if(theSink.confirmation)
	{
		LOGD("auth: Free Confirmation Addr\n");
		freePanic(theSink.confirmation_addr);
	}
	theSink.confirmation_addr = NULL;
	theSink.confirmation = FALSE;
}

