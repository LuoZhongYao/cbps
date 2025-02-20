/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    bluestack_handler.c

DESCRIPTION
    Handles primitives received by BlueStack and routes them to the
    appropriate handler function depending on their type.

NOTES

*/



/****************************************************************************
    Header files
*/
#include "connection.h"
#include "connection_private.h"
#include "init.h"
#include "bluestack_handler.h"
#include "dm_baseband_handler.h"
#include "dm_dut_handler.h"
#include "dm_info_handler.h"
#include "dm_inquiry_handler.h"
#include "dm_read_tx_power_handler.h"
#include "dm_sync_handler.h"
#include "dm_security_handler.h"
#include "l2cap_handler.h"
#include "rfc_handler.h"
#include "sdp_handler.h"
#include "dm_link_policy_handler.h"
#include "dm_bad_message_handler.h"
#include "dm_page_timeout_handler.h"

#ifndef DISABLE_BLE
#include "dm_ble_handler.h"
#include "dm_ble_security.h"
#include "dm_ble_random_address.h"
#include "dm_ble_connection_parameters_update.h"
#include "dm_ble_advertising.h"
#endif

#include <app/bluestack/types.h>
#include <app/bluestack/dm_prim.h>
#include <app/bluestack/l2cap_prim.h>
#include <app/bluestack/rfcomm_prim.h>
#include <app/bluestack/sdc_prim.h>
#include <app/bluestack/sds_prim.h>


#include <print.h>
#include <vm.h>

/*lint -e655 -e525 -e830 */

/* Connection state management */
#define SET_CM_STATE(s) theCm->state = (s);

/* List of reason code for default handling of unexpected messages */
typedef enum
{
    connectionUnexpectedCmPrim,
    connectionUnexpectedDmPrim,
    connectionUnexpectedRfcPrim,
    connectionUnexpectedSdpPrim,
    connectionUnexpectedL2capPrim,
    connectionUnhandledMessage
}connectionUnexpectedReasonCode;

/****************************************************************************
NAME
    handleUnexpected

DESCRIPTION
    This macro is called as a result of a message arriving when the
    Connection Library was not expecting it.

RETURNS
    void
*/
#define handleUnexpected(code, state, type) \
    CL_DEBUG(("handleUnexpected - Code 0x%x State 0x%x MsgId 0x%x\n", code, state, type))

/****************************************************************************
NAME
    connectionBluestackHandlerDm

DESCRIPTION
    Message handler for the DM primitives.

RETURNS
    void
*/
static void connectionBluestackHandlerDm(connectionState *theCm, DM_UPRIM_T *message)
{
    const u8 state = theCm->state;
    if (state == connectionReady)
    {
        switch (message->type)
        {
#ifndef CL_EXCLUDE_SYNC
            case DM_SYNC_REGISTER_CFM:
                PRINT(("DM_SYNC_REGISTER_CFM\n"));
                connectionHandleSyncRegisterCfm( (DM_SYNC_REGISTER_CFM_T *)message);
                return;

            case DM_SYNC_UNREGISTER_CFM:
                PRINT(("DM_SYNC_UNREGISTER_CFM\n"));
                connectionHandleSyncUnregisterCfm( (DM_SYNC_UNREGISTER_CFM_T *)message );
                return;

            case DM_SYNC_CONNECT_CFM:
                PRINT(("DM_SYNC_CONNECT_CFM\n"));
                connectionHandleSyncConnectCfm(theCm->theAppTask, (DM_SYNC_CONNECT_CFM_T *) message);
                return;

            case DM_SYNC_CONNECT_COMPLETE_IND:
                PRINT(("DM_SYNC_CONNECT_COMPLETE_IND\n"));
                connectionHandleSyncConnectCompleteInd(theCm->theAppTask, (DM_SYNC_CONNECT_COMPLETE_IND_T *) message);
                return;

            case DM_SYNC_CONNECT_IND:
                PRINT(("DM_SYNC_CONNECT_IND\n"));
                connectionHandleSyncConnectInd((DM_SYNC_CONNECT_IND_T *) message);
                return;

            case DM_SYNC_DISCONNECT_IND:
                PRINT(("DM_SYNC_DISCONNECT_IND\n"));
                connectionHandleSyncDisconnectInd((DM_SYNC_DISCONNECT_IND_T *) message);
                return;

            case DM_SYNC_DISCONNECT_CFM:
                PRINT(("DM_SYNC_DISCONNECT_CFM\n"));
                connectionHandleSyncDisconnectCfm((DM_SYNC_DISCONNECT_CFM_T *) message);
                return;

            case DM_SYNC_RENEGOTIATE_IND:
                PRINT(("DM_SYNC_RENEGOTIATE_IND\n"));
                connectionHandleSyncRenegotiateInd((DM_SYNC_RENEGOTIATE_IND_T *) message);
                return;

            case DM_SYNC_RENEGOTIATE_CFM: /* 31783 */
                PRINT(("DM_SYNC_RENEGOTIATE_CFM\n"));
                connectionHandleSyncRenegotiateCfm((DM_SYNC_RENEGOTIATE_CFM_T *) message);
                return;
#endif
        }

        switch (message->type)
        {
            case DM_SM_REGISTER_CFM:
                PRINT(("DM_SM_REGISTER_CFM\n"));
                /* Not currently handled */
                return;

            case DM_SM_UNREGISTER_CFM:
                PRINT(("DM_SM_UNREGISTER_CFM\n"));
                /* Not currently handled */
                return;

            case DM_SM_REGISTER_OUTGOING_CFM:
                PRINT(("DM_SM_REGISTER_OUTGOING_CFM\n"));
                handleRegisterOutgoingCfm((DM_SM_REGISTER_OUTGOING_CFM_T *)message);
                return;

            case DM_SM_UNREGISTER_OUTGOING_CFM:
                PRINT(("DM_SM_UNREGISTER_OUTGOING_CFM\n"));
                /* Not currently handled */
                return;

            case DM_SM_ADD_DEVICE_CFM: /* 11284 */
                PRINT(("DM_SM_ADD_DEVICE_CFM\n"));
                connectionHandleSmAddDeviceCfmReady(&theCm->smState, (DM_SM_ADD_DEVICE_CFM_T*)message);
                return;

            case DM_SM_PIN_REQUEST_IND: /* 11287 */
                PRINT(("DM_SM_PIN_REQUEST_IND\n"));
                connectionHandleSmPinReqInd(theCm->theAppTask, (DM_SM_PIN_REQUEST_IND_T*)message);
                return;
            case DM_SM_KEYS_IND: /* 11288 */
                PRINT(("DM_SM_KEYS_IND\n"));
                connectionHandleSmKeysInd(theCm->theAppTask, &theCm->smState, (DM_SM_KEYS_IND_T*)message);
                return;
            case DM_SM_AUTHORISE_IND: /* 11289 */
                PRINT(("DM_SM_AUTHORISE_IND\n"));
                connectionHandleSmAuthoriseInd(theCm->theAppTask, (DM_SM_AUTHORISE_IND_T*)message);
                return;

            case DM_SM_AUTHENTICATE_CFM: /* 11290 */
                PRINT(("DM_SM_AUTHENTICATE_CFM\n"));
                return;

            case DM_SM_ENCRYPT_CFM: /* 11291 */
                PRINT(("DM_SM_ENCRYPT_CFM\n"));
                connectionHandleEncryptCfm(&theCm->smState, (DM_SM_ENCRYPT_CFM_T *) message);
                return;

            case DM_SM_ENCRYPTION_CHANGE_IND: /* 11292 */
                PRINT(("DM_SM_ENCRYPTION_CHANGE_IND\n"));
                connectionHandleEncryptionChange((DM_SM_ENCRYPTION_CHANGE_IND_T *)message);
                return;

            case DM_SM_READ_LOCAL_OOB_DATA_CFM: /* 11295 */
                PRINT(("DM_SM_READ_LOCAL_OOB_DATA_CFM\n"));
                connectionHandleReadLocalOobDataCfm(&theCm->smState, (DM_SM_READ_LOCAL_OOB_DATA_CFM_T*)message);
                return;

            case DM_SM_IO_CAPABILITY_RESPONSE_IND: /* 11297 */
                PRINT(("DM_SM_IO_CAPABILITY_RESPONSE_IND \n"));
                connectionHandleSmIoCapResInd(theCm->theAppTask, &theCm->smState, (DM_SM_IO_CAPABILITY_RESPONSE_IND_T*)message);
                return;

            case DM_SM_IO_CAPABILITY_REQUEST_IND: /* 11298 */
                PRINT(("DM_SM_IO_CAPABILITY_REQUEST_IND \n"));
                connectionHandleSmIoCapReqInd(theCm->theAppTask, &theCm->smState, (DM_SM_IO_CAPABILITY_REQUEST_IND_T*)message);
                return;

            case DM_SM_SIMPLE_PAIRING_COMPLETE_IND: /* 11299 */
                PRINT(("DM_SM_SIMPLE_PAIRING_COMPLETE_IND\n"));
                connectionHandleSmSimplePairingCompleteInd(theCm->theAppTask, &theCm->smState, (DM_SM_SIMPLE_PAIRING_COMPLETE_IND_T*)message);
                return;

            case DM_SM_USER_CONFIRMATION_REQUEST_IND: /* 11300 */
                PRINT(("DM_SM_USER_CONFIRMATION_REQUEST_IND \n"));
                connectionHandleSmUserConfirmationReqInd(theCm->theAppTask, (DM_SM_USER_CONFIRMATION_REQUEST_IND_T*)message);
                return;

            case DM_SM_USER_PASSKEY_REQUEST_IND: /* 11301 */
                PRINT(("DM_SM_USER_PASSKEY_REQUEST_IND \n"));
                connectionHandleSmUserPasskeyReqInd(theCm->theAppTask, (DM_SM_USER_PASSKEY_REQUEST_IND_T*)message);
                return;

            case DM_SM_USER_PASSKEY_NOTIFICATION_IND: /* 11303 */
                PRINT(("DM_SM_USER_PASSKEY_NOTIFICATION_IND \n"));
                connectionHandleSmUserPasskeyNotificationInd(theCm->theAppTask, &theCm->smState, (DM_SM_USER_PASSKEY_NOTIFICATION_IND_T*)message);
                return;

            case DM_SM_KEYPRESS_NOTIFICATION_IND: /* 11304 */
                PRINT(("DM_SM_KEYPRESS_NOTIFICATION_IND \n"));
                connectionHandleSmKeypressNotificationInd(theCm->theAppTask, (DM_SM_KEYPRESS_NOTIFICATION_IND_T*)message);
                return;

            case DM_SM_KEY_REQUEST_IND:
                PRINT(("DM_SM_KEY_REQUEST_IND \n"));
                connectionHandleSmKeyReqInd(theCm->theAppTask, &theCm->smState, (DM_SM_KEY_REQUEST_IND_T *)message);
                return;

#ifndef DISABLE_BLE
            case DM_SM_SECURITY_CFM:
                PRINT(("DM_SM_SECURITY_CFM\n"));
                connectionHandleDmSmSecurityCfm(
                    (DM_SM_SECURITY_CFM_T *)message
                    );
                return;

            case DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM:
                PRINT(("DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM\n"));
                connectionHandleDmBleUpdateConnectionParametersCfm(
                    &theCm->l2capState,
                    (DM_BLE_UPDATE_CONNECTION_PARAMETERS_CFM_T*)message);
                return;

            case DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND:
                PRINT(("DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND\n"));
                connectionHandleDmBleAcceptConnectionParUpdateInd(
                    (DM_BLE_ACCEPT_CONNECTION_PAR_UPDATE_IND_T*)message
                    );
                return;

            case DM_ULP_ADV_PARAM_UPDATE_IND:
                PRINT(("DM_ULP_ADV_PARAM_UPDATE_IND\n"));
                connectionHandleDmBleAdvParamUpdateInd(
                        (DM_ULP_ADV_PARAM_UPDATE_IND_T *)message
                        );
                return;
#endif

        }

        switch (message->type)
        {

#ifndef CL_EXCLUDE_INQUIRY

            case DM_HCI_PERIODIC_INQUIRY_MODE_CFM: /* 1058 */
                PRINT(("DM_HCI_PERIODIC_INQUIRY_COMPLETE\n"));
                return;

            case DM_HCI_EXIT_PERIODIC_INQUIRY_MODE_CFM: /* 1059 */
                PRINT(("DM_HCI_EXIT_PERIODIC_INQUIRY_CFM\n"));
                connectionHandleExitPeriodicInquiryComplete(&theCm->inqState);
                return;

            case DM_HCI_INQUIRY_CANCEL_CFM: /* 1057 */
                PRINT(("DM_HCI_INQUIRY_CANCEL_CFM\n"));
                connectionHandleInquiryComplete(&theCm->inqState);
                return;

            case DM_HCI_INQUIRY_RESULT_IND:    /* 1060 */
                PRINT(("DM_HCI_INQUIRY_RESULT_IND\n"));
                connectionHandleInquiryResult(&theCm->inqState, (DM_HCI_INQUIRY_RESULT_IND_T *)message);
                return;

            case DM_HCI_INQUIRY_CFM: /* 1061 */
                PRINT(("DM_HCI_INQUIRY_CFM\n"));
                connectionHandleInquiryComplete(&theCm->inqState);
                return;
#endif

            case DM_HCI_REMOTE_NAME_CFM: /* 1065 */
                PRINT(("DM_HCI_REMOTE_NAME_CFM\n"));
                connectionHandleRemoteNameComplete(&theCm->inqState, (DM_HCI_REMOTE_NAME_CFM_T *) message);
                return;

            case DM_HCI_REMOTE_NAME_REQ_CANCEL_CFM:
                PRINT(("DM_HCI_REMOTE_NAME_CANCEL_CFM\n"));
                connectionHandleRemoteNameCancelCfm(&theCm->inqState,
                         (DM_HCI_REMOTE_NAME_REQ_CANCEL_CFM_T *) message);
                return;


            case DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM: /* 1066 */
                PRINT(("DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM\n"));
                connectionHandleReadRemoteSupportedFeaturesCfm(&theCm->infoState, (DM_HCI_READ_REMOTE_SUPP_FEATURES_CFM_T*) message);
                return;

            case DM_HCI_READ_REMOTE_VER_INFO_CFM: /* 1067 */
                PRINT(("DM_HCI_READ_REMOTE_VER_INFO_CFM\n"));
                connectionHandleReadRemoteVersionCfm(&theCm->infoState, (DM_HCI_READ_REMOTE_VER_INFO_CFM_T*) message);
                return;

            case DM_HCI_READ_CLOCK_OFFSET_CFM: /* 1068 */
                PRINT(("DM_HCI_READ_CLOCK_OFFSET_CFM\n"));
                connectionHandleReadClkOffsetComplete(&theCm->infoState, (DM_HCI_READ_CLOCK_OFFSET_CFM_T *) message);
                return;

            case DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND:
                PRINT(("DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND\n"));
                connectionHandleAPTExpiredInd(theCm->theAppTask, (DM_HCI_AUTHENTICATED_PAYLOAD_TIMEOUT_EXPIRED_IND_T *) message);
                return;
        }

        switch (message->type)
        {

#ifndef CL_EXCLUDE_INQUIRY

            case DM_HCI_READ_INQUIRY_MODE_CFM: /* 30730 */
                PRINT(("DM_HCI_READ_INQUIRY_MODE_CFM\n"));
                connectionHandleReadInquiryModeComplete(&theCm->inqState, (DM_HCI_READ_INQUIRY_MODE_CFM_T *)message);
                return;

            case DM_HCI_WRITE_INQUIRY_MODE_CFM: /* 30731 */
                PRINT(("DM_HCI_WRITE_INQUIRY_MODE_CFM\n"));
                connectionHandleWriteInquiryModeComplete(&theCm->inqState, (DM_HCI_WRITE_INQUIRY_MODE_CFM_T *)message);
                return;

            case DM_HCI_INQUIRY_RESULT_WITH_RSSI_IND: /* 30742 */
                PRINT(("DM_HCI_INQUIRY_RESULT_WITH_RSSI_IND\n"));
                connectionHandleInquiryResultWithRssi(&theCm->inqState, (DM_HCI_INQUIRY_RESULT_WITH_RSSI_IND_T*)message);
                return;
#endif

            case DM_HCI_SNIFF_SUB_RATING_IND: /* 30784 */
                PRINT(("DM_HCI_SNIFF_SUB_RATING_IND\n"));
                connectionHandleSniffSubRatingInd((DM_HCI_SNIFF_SUB_RATING_IND_T *)message);
                return;


#ifndef CL_EXCLUDE_INQUIRY

            case DM_HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA_CFM: /* 30786 */
                PRINT(("DM_HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA_CFM\n"));
                connectionHandleReadEirDataComplete(&theCm->inqState, (DM_HCI_READ_EXTENDED_INQUIRY_RESPONSE_DATA_CFM_T *)message);
                return;

            case DM_HCI_EXTENDED_INQUIRY_RESULT_IND: /* 30788 */
                PRINT(("DM_HCI_EXTENDED_INQUIRY_RESULT_IND_T\n"));
                connectionHandleExtendedInquiryResult(&theCm->inqState, (DM_HCI_EXTENDED_INQUIRY_RESULT_IND_T *)message);
                return;

            case DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_CFM: /* 30790 */
                PRINT(("DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_CFM\n"));
                connectionHandleReadInquiryTxComplete(&theCm->inqState, (DM_HCI_READ_INQUIRY_RESPONSE_TX_POWER_LEVEL_CFM_T *) message);
                return;
#endif

            case DM_HCI_READ_TX_POWER_LEVEL_CFM:
                 PRINT(("DM_HCI_READ_TX_POWER_LEVEL_CFM\n"));
                 connectionHandleReadTxPowerComplete(&theCm->readTxPwrState,
                                        (DM_HCI_READ_TX_POWER_LEVEL_CFM_T*)message);
                 return;

            case DM_HCI_LINK_SUPERV_TIMEOUT_IND: /* 30792 */
                PRINT(("DM_HCI_LINK_SUPERV_TIMEOUT_IND\n"));
                connectionHandleLinkSupervisionTimeoutInd((DM_HCI_LINK_SUPERV_TIMEOUT_IND_T *)message);
                return;

            case DM_HCI_REFRESH_ENCRYPTION_KEY_IND: /* 30794 */
                PRINT(("DM_HCI_REFRESH_ENCRYPTION_KEY_IND\n"));
                connectionHandleEncryptionKeyRefreshInd((DM_HCI_REFRESH_ENCRYPTION_KEY_IND_T*) message);
                return;
        }

        switch (message->type)
        {
#ifndef CL_EXCLUDE_L2CAP
            case DM_HCI_QOS_SETUP_CFM: /* 2064 */
                PRINT(("DM_HCI_QOS_SETUP_CFM\n"));
                connectionHandleQosSetupCfm((DM_HCI_QOS_SETUP_CFM_T *) message);
                return;
#endif

            case DM_HCI_ROLE_DISCOVERY_CFM: /* 2066 */
                PRINT(("DM_HCI_ROLE_DISCOVERY_CFM\n"));
                connectionHandleRoleDiscoveryComplete(&theCm->linkPolicyState, (DM_HCI_ROLE_DISCOVERY_CFM_T *) message);
                return;

            case DM_HCI_SWITCH_ROLE_CFM: /* 2067 */
                PRINT(("DM_HCI_SWITCH_ROLE_CFM\n"));
                connectionHandleDmSwitchRoleComplete(theCm->theAppTask, &theCm->linkPolicyState, (DM_HCI_SWITCH_ROLE_CFM_T *) message);
                return;

            case DM_HCI_READ_LOCAL_NAME_CFM: /* 3147 */
                PRINT(("DM_HCI_READ_LOCAL_NAME_CFM\n"));
                connectionHandleLocalNameComplete(&theCm->inqState, (DM_HCI_READ_LOCAL_NAME_CFM_T *) message);
                return;

            case DM_HCI_READ_CLASS_OF_DEVICE_CFM: /* 3160 */
                PRINT(("DM_HCI_READ_CLASS_OF_DEVICE_CFM\n"));
                connectionHandleReadClassOfDeviceComplete(&theCm->infoState, (DM_HCI_READ_CLASS_OF_DEVICE_CFM_T *)message);
                return;


#ifndef CL_EXCLUDE_INQUIRY
            case DM_HCI_WRITE_CURRENT_IAC_LAP_CFM: /* 3177 */
                PRINT(("DM_HCI_WRITE_CURRENT_IAC_LAP_CFM\n"));
                connectionHandleWriteIacLapComplete(&theCm->inqState, (DM_HCI_WRITE_CURRENT_IAC_LAP_CFM_T *)message);
                return;
#endif

            case DM_HCI_READ_BD_ADDR_CFM: /* 4110 */
                PRINT(("DM_HCI_READ_BD_ADDR_CFM\n"));
                connectionHandleReadBdAddrComplete(&theCm->infoState, (DM_HCI_READ_BD_ADDR_CFM_T *)message);
                return;

            case DM_HCI_GET_LINK_QUALITY_CFM: /* 5129 */
                PRINT(("DM_HCI_GET_LINK_QUALITY_CFM\n"));
                connectionHandleReadLinkQualityComplete(&theCm->infoState, (DM_HCI_GET_LINK_QUALITY_CFM_T *) message);
                return;

            case DM_HCI_READ_RSSI_CFM: /* 5130 */
                PRINT(("DM_HCI_READ_RSSI_CFM\n"));
                connectionHandleReadRssiComplete(&theCm->infoState, (DM_HCI_READ_RSSI_CFM_T *) message);
                return;

            case DM_LP_WRITE_POWERSTATES_CFM: /* 30977 */
                PRINT(("DM_LP_WRITE_POWERSTATES_CFM\n"));
                connectionLinkPolicyHandleWritePowerStatesCfm((DM_LP_WRITE_POWERSTATES_CFM_T *)message);
                return;
        }
    }

    if (state != connectionUninitialised)
    {
        if (state != connectionInitialising)
        {
            /* e.g. state is connectionReady or connectionTestMode */

            switch (message->type)
            {
                case DM_HCI_MODE_CHANGE_EVENT_IND: /* 2063 */
                    PRINT(("DM_HCI_MODE_CHANGE_EVENT_IND\n"));
                    connectionHandleDmHciModeChangeEvent(theCm->theAppTask, (DM_HCI_MODE_CHANGE_EVENT_IND_T *)message);
                    return;

                case DM_ACL_OPEN_CFM: /* 10252 */
                    PRINT(("DM_ACL_OPEN_CFM\n"));
                    connectionHandleDmAclOpenCfm(&theCm->smState, (DM_ACL_OPEN_CFM_T*)message);
                    return;

                case DM_ACL_OPENED_IND: /* 10253 */
                    PRINT(("DM_ACL_OPENED_IND\n"));
                    connectionHandleDmAclOpenInd(theCm->theAppTask, &theCm->smState, (DM_ACL_OPENED_IND_T *) message);
                    return;

                case DM_ACL_CLOSED_IND: /* 28697 */
                    PRINT(("DM_ACL_CLOSED_IND\n"));
                    connectionHandleDmAclClosedInd(theCm->theAppTask, &theCm->smState, (DM_ACL_CLOSED_IND_T*)message);
                    return;

                case DM_SM_BONDING_CFM: /*B-56544 */
                    PRINT(("DM_SM_BONDING_CFM\n"));
                    connectionHandleSmBondingCfm(theCm->theAppTask, &theCm->smState, (DM_SM_BONDING_CFM_T*)message);
                    return;
            }
        }

        /* state can be connectionInitialising, connectionReady OR connectionTestMode */

        switch (message->type)
        {
            case DM_SET_BT_VERSION_CFM:
                PRINT(("DM_SET_BT_VERSION_CFM\n"));
                connectionHandleSetBtVersionCfm(&theCm->infoState, (DM_SET_BT_VERSION_CFM_T*)message);
                return;

            case DM_HCI_READ_LOCAL_VER_INFO_CFM:
                PRINT(("DM_HCI_READ_LOCAL_VER_INFO_CFM\n"));
                connectionHandleReadLocalVersionCfm(&theCm->infoState, (DM_HCI_READ_LOCAL_VER_INFO_CFM_T *) message);
                return;

            case DM_AM_REGISTER_CFM:
                PRINT(("DM_AM_REGISTER_CFM\n"));
                if (state == connectionInitialising)
                {
                    connectionSendInternalInitCfm(connectionInitDm);
                    return;
                }
                break;

            case DM_SM_ADD_DEVICE_CFM:
                PRINT(("DM_SM_ADD_DEVICE_CFM\n"));
                if (state == connectionInitialising)
                {
                    connectionHandleSmAddDeviceCfm(&theCm->smState, (DM_SM_ADD_DEVICE_CFM_T*)message);
                    return;
                }
                break;

            case DM_SM_ACCESS_IND:
                PRINT(("DM_SM_ACCESS_IND\n"));
                if (state != connectionTestMode)
                {
                    connectionHandleSmAccessInd(&theCm->sdpState, (DM_SM_ACCESS_IND_T*)message);
                    return;
                }
                break;

            /* The DM_PRIMS:
                    DM_SM_SET_DEFAULT_SECURITY_REQ
                    DM_SM_SET_SEC_MODE_REQ
                    DM_SM_SET_SEC_MODE_CFM
                    DM_SM_SEC_MODE_CONFIG_REQ
                    DM_SM_SEC_MODE_CONFIG_CFM

               Have been deprecated and replaced by
                    DM_SM_INIT_REQ
                    DM_SM_INIT_CFM

               Connection library maintains the upward API however (except
               during initialisation) for the functions:
                    ConnectionSmSetSecurityMode
                    ConnectionSmSecModeConfig
                    ConnectionSmSetSecurityLevel
             */
            case DM_SM_INIT_CFM:
                PRINT(("DM_SM_INIT_CFM\n"));
                {
                    if (state == connectionInitialising)
                    {
                        /* During library intialisation, this message can only
                           be in response to a connectionSmInit function call.
                         */
                        handleSecurityInitCfm(
                            &theCm->infoState,
                            (DM_SM_INIT_CFM_T *)message
                            );
                    }
                    else if (state == connectionReady)
                    {
                        switch (theCm->smState.sm_init_msg)
                        {
                            case sm_init_set_security_mode:
                                connectionHandleSetSecurityModeCfm(
                                    &theCm->smState,
                                    (DM_SM_INIT_CFM_T *)message
                                    );
                                break;
                            case sm_init_set_security_config:
                                connectionHandleConfigureSecurityCfm(
                                    &theCm->smState,
                                    (DM_SM_INIT_CFM_T *)message);
                                break;
                            case sm_init_set_security_default:
                                /* Connection/App expects no cfm for this */
                                break;
                            default:
                                Panic();
                                break;
                        }
                    }
                    else /* (state == connectionTestMode) */
                    {
                        /* Hijack the DM_SM_INIT_CFM response for the call to
                         ConnectionSmSetSecurityMode in
                         connectionHandleEnterDutModeReq
                         */

                        connectionHandleDutSecurityDisabled(
                            theCm->theAppTask,
                            (DM_SM_INIT_CFM_T *)message
                            );
                }

                    theCm->smState.sm_init_msg = sm_init_set_none;
                    return;
                }
                break;

            case DM_HCI_ENABLE_DUT_MODE_CFM:
                PRINT(("DM_HCI_ENABLE_DUT_MODE_CFM\n"));
                if (state == connectionTestMode)
                {
                    connectionHandleDutCfm(theCm->theAppTask, (DM_HCI_ENABLE_DUT_MODE_CFM_T *) message);
                    return;
                }
                break;

#ifndef DISABLE_BLE
            case DM_HCI_ULP_ADVERTISING_REPORT_IND:
                PRINT(("DM_HCI_ULP_ADVERTISING_REPORT_IND\n"));
                connectionHandleDmBleAdvertisingReportInd(
                        (DM_HCI_ULP_ADVERTISING_REPORT_IND_T *)message
                        );
                return;

            case DM_HCI_ULP_SET_ADVERTISING_DATA_CFM:
                PRINT(("DM_HCI_ULP_SET_ADVERTISING_DATA_CFM\n"));
                connectionHandleDmBleSetAdvertisingDataCfm(
                        (DM_HCI_ULP_SET_ADVERTISING_DATA_CFM_T *)message
                        );
                return;

            case DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM:
                PRINT(("DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM\n"));
                connectionHandleDmBleSetAdvertisingParamsCfm(
                    (DM_HCI_ULP_SET_ADVERTISING_PARAMETERS_CFM_T*) message);
                return;

            case DM_SET_BLE_CONNECTION_PARAMETERS_CFM:
                PRINT(("DM_SET_BLE_CONNECTION_PARAMETERS_CFM\n"));
                connectionHandleDmSetBleConnectionParametersCfm(
                    (DM_SET_BLE_CONNECTION_PARAMETERS_CFM_T*)message);
                return;

            case DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM:
                PRINT(("DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM\n"));
                connectionHandleDmBleSetScanParamatersCfm(
                        (DM_HCI_ULP_SET_SCAN_PARAMETERS_CFM_T*)message
                        );
                return;

            case DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_CFM:
                PRINT(("DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_CFM\n"));
                connectionHandleDmBleSetScanResponseDataCfm(
                        (DM_HCI_ULP_SET_SCAN_RESPONSE_DATA_CFM_T*)message
                        );
                return;

            case DM_HCI_ULP_READ_WHITE_LIST_SIZE_CFM:
                PRINT(("DM_HCI_ULP_READ_WHITE_LIST_SIZE_CFM\n"));
                connectionHandleDmBleReadWhiteListSizeCfm(
                        (DM_HCI_ULP_READ_WHITE_LIST_SIZE_CFM_T*)message
                        );
                return;

            case DM_HCI_ULP_CLEAR_WHITE_LIST_CFM:
                PRINT(("DM_HCI_ULP_CLEAR_WHITE_LIST_CFM\n"));
                connectionHandleDmBleClearWhiteListCfm(
                        (DM_HCI_ULP_CLEAR_WHITE_LIST_CFM_T*)message
                        );
                return;

            case DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM:
                PRINT(("DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM\n"));
                connectionHandleDmBleAddDeviceToWhiteListCfm(
                        (DM_HCI_ULP_ADD_DEVICE_TO_WHITE_LIST_CFM_T*)message
                        );
                return;

            case DM_HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_CFM:
                PRINT(("DM_HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_CFM\n"));
                connectionHandleDmBleRemoveDeviceFromWhiteListCfm(
                        (DM_HCI_ULP_REMOVE_DEVICE_FROM_WHITE_LIST_CFM_T*)message
                        );
                return;

            case DM_SM_CONFIGURE_LOCAL_ADDRESS_CFM:
                PRINT(("DM_SM_CONFIGURE_LOCAL_ADDRESS_CFM\n"));
                connectionHandleDmSmConfigureLocalAddressCfm(
                        (DM_SM_CONFIGURE_LOCAL_ADDRESS_CFM_T*)message
                        );
                return;

#endif /* DISABLE_BLE */

            /* In the debug version of the lib check the status otherwise ignore. These prims all return
               a message which is essentially a DM_HCI_STANDARD_COMMAND_COMPLETE so we handle them all
               in the same way.    */
            case DM_HCI_WRITE_CLASS_OF_DEVICE_CFM:
            case DM_HCI_WRITE_PAGESCAN_ACTIVITY_CFM:
            case DM_HCI_WRITE_INQUIRYSCAN_ACTIVITY_CFM:
            case DM_HCI_WRITE_INQUIRY_SCAN_TYPE_CFM:
            case DM_HCI_WRITE_PAGE_SCAN_TYPE_CFM:
            case DM_HCI_WRITE_SCAN_ENABLE_CFM:
            case DM_HCI_SET_EVENT_FILTER_CFM:
            case DM_WRITE_CACHED_PAGE_MODE_CFM:
            case DM_WRITE_CACHED_CLOCK_OFFSET_CFM:
            case DM_CLEAR_PARAM_CACHE_CFM:
            case DM_HCI_CHANGE_LOCAL_NAME_CFM:
            case DM_HCI_WRITE_INQUIRY_TRANSMIT_POWER_LEVEL_CFM:
            case DM_HCI_WRITE_EXTENDED_INQUIRY_RESPONSE_DATA_CFM:
            case DM_HCI_SNIFF_SUB_RATE_CFM:
#ifndef DISABLE_BLE
            case DM_HCI_ULP_SET_SCAN_ENABLE_CFM:
            case DM_HCI_ULP_SET_ADVERTISE_ENABLE_CFM:
#endif /* DISABLE_BLE */
                checkStatus(message);
                return;

            /* Primitives we ignore. These have a specific return message which we cannot generalise
               with the macro we use for the primitives above so for the moment ignore these. */
            case DM_SM_REMOVE_DEVICE_CFM:
            case DM_HCI_READ_LOCAL_SUPP_FEATURES_CFM:
            case DM_HCI_DELETE_STORED_LINK_KEY_CFM:
            case DM_HCI_WRITE_LINK_SUPERV_TIMEOUT_CFM:
            case DM_HCI_WRITE_DEFAULT_LINK_POLICY_SETTINGS_CFM:
            case DM_HCI_WRITE_AUTO_FLUSH_TIMEOUT_CFM:
            case DM_HCI_CHANGE_CONN_LINK_KEY_CFM:
            case DM_HCI_CREATE_CONNECTION_CANCEL_CFM:
            case DM_HCI_READ_REMOTE_EXT_FEATURES_CFM:
                return;
        }
    }

    if (message->type == DM_BAD_MESSAGE_IND)
    {
       connectionHandleDmBadMessageInd((DM_BAD_MESSAGE_IND_T *) message);
       return;
    }

    /* Prims we are not handling - for now panic the app */
    handleUnexpected(connectionUnexpectedDmPrim, state, message->type);
}

/****************************************************************************
NAME
    connectionBluestackHandlerReadyL2cap

DESCRIPTION
    L2CAP message handler for the ready state.

RETURNS
    void
*/
#if !defined(CL_EXCLUDE_L2CAP) || !defined(DISABLE_BLE)
static void connectionBluestackHandlerL2cap(connectionState *theCm, L2CA_UPRIM_T *message)
{
    if (theCm->state == connectionReady)
    {
        switch (message->type)
        {
#ifndef CL_EXCLUDE_L2CAP
            case L2CA_REGISTER_CFM:
                PRINT(("L2CA_REGISTER_CFM\n"));
                connectionHandleL2capRegisterCfm((L2CA_REGISTER_CFM_T *)message);
                return;

            case L2CA_UNREGISTER_CFM:
                PRINT(("L2CA_UNREGISTER_CFM\n"));
                connectionHandleL2capUnregisterCfm((L2CA_UNREGISTER_CFM_T *) message);
                return;

            case L2CA_AUTO_CONNECT_IND:
                PRINT(("L2CA_CONNECT_IND\n"));
                connectionHandleL2capConnectInd((L2CA_AUTO_CONNECT_IND_T *) message);
                return;

            case L2CA_AUTO_CONNECT_CFM:
                PRINT(("L2CA_CONNECT_CFM\n"));
                connectionHandleL2capConnectCfm((L2CA_AUTO_CONNECT_CFM_T *)message);
                return;

            case L2CA_CONFIG_IND:
            case L2CA_CONFIG_CFM:
                PRINT(("L2CA_CONFIG_IND or L2CA_CONFIG_CFM\n"));
                Panic();        /* shouldn't get here */
                break;

            case L2CA_DISCONNECT_IND:
                PRINT(("L2CA_DISCONNECT_IND\n"));
                connectionHandleL2capDisconnectInd((L2CA_DISCONNECT_IND_T *) message);
                return;

            case L2CA_DISCONNECT_CFM:
                PRINT(("L2CA_DISCONNECT_CFM\n"));
                connectionHandleL2capDisconnectCfm((L2CA_DISCONNECT_CFM_T *) message);
                return;

            case L2CA_TIMEOUT_IND:
                PRINT(("L2CA_TIMEOUT_IND\n"));
                /* A disconnect is always indicated, usually before the TIMEOUT. */
                return;

            case L2CA_GETINFO_CFM:
                PRINT(("L2CA_GETINFO_CFM\n"));
                connectionHandleL2capGetInfoCfm(&theCm->l2capState, (L2CA_GETINFO_CFM_T *)message);
                return;

            case L2CA_MAP_FIXED_CID_CFM:
                PRINT(("L2CA_MAP_FIXED_CID_CFM\n"));
                connectionHandleL2capMapFixedCidCfm(&theCm->l2capState, (L2CA_MAP_FIXED_CID_CFM_T *) message);
                return;

            case L2CA_MAP_FIXED_CID_IND:
                PRINT(("L2CA_MAP_FIXED_CID_IND\n"));
                connectionHandleL2capMapFixedCidInd((L2CA_MAP_FIXED_CID_IND_T *) message);
                return;

            case L2CA_UNMAP_FIXED_CID_IND:
                PRINT(("L2CA_UNMAP_FIXED_CID_IND\n"));
                connectionHandleL2capUnmapFixedCidInd((L2CA_UNMAP_FIXED_CID_IND_T *) message);
                return;

            case L2CA_RAW_DATA_IND:
                free( VmGetPointerFromHandle(
                            ((L2CA_RAW_DATA_IND_T*)message)->data
                            )
                    );
                return;
#endif

        }
    }

    /* Prims we are not handling - for now panic the app */
    handleUnexpected(connectionUnexpectedL2capPrim, theCm->state, message->type);
}
#endif

/****************************************************************************
NAME
    connectionBluestackHandlerReadyRfcomm

DESCRIPTION
    Rfcomm message handler for the ready state.

RETURNS
    void
*/
#ifndef CL_EXCLUDE_RFCOMM
static void connectionBluestackHandlerRfcomm(connectionState *theCm, RFCOMM_UPRIM_T *message)
{
    if (theCm->state == connectionReady)
    {
        switch (message->type)
        {
            case RFC_REGISTER_CFM:
                PRINT(("RFC_REGISTER_CFM\n"));
                connectionHandleRfcommRegisterCfm((RFC_REGISTER_CFM_T*)message);
                return;

            case RFC_UNREGISTER_CFM:
                PRINT(("RFC_UNREGISTER_CFM\n"));
                connectionHandleRfcommUnregisterCfm((RFC_UNREGISTER_CFM_T*)message);
                return;

            case RFC_CLIENT_CONNECT_CFM:
                PRINT(("RFC_CLIENT_CONNECT_CFM\n"));
                connectionHandleRfcommClientConnectCfm((RFC_CONNECT_CFM_T*)message);
                return;

            case RFC_SERVER_CONNECT_CFM:
                PRINT(("RFC_SERVER_CONNECT_CFM\n"));
                connectionHandleRfcommServerConnectCfm((RFC_SERVER_CONNECT_CFM_T*)message);
                return;

            case RFC_SERVER_CONNECT_IND:
                PRINT(("RFC_SERVER_CONNECT_IND\n"));
                connectionHandleRfcommConnectInd((RFC_SERVER_CONNECT_IND_T*)message);
                return;

            case RFC_DISCONNECT_IND:
                PRINT(("RFC_DISCONNECT_IND"));
                connectionHandleRfcommDisconnectInd((RFC_DISCONNECT_IND_T*)message);
                return;

            case RFC_DISCONNECT_CFM:
                PRINT(("RFC_DISCONNECT_CFM"));
                connectionHandleRfcommDisconnectCfm((RFC_DISCONNECT_CFM_T*)message);
                return;

            case RFC_PORTNEG_CFM:
                PRINT(("RFC_PORTNEG_CFM"));
                connectionHandleRfcommPortNegCfm((RFC_PORTNEG_CFM_T*)message);
                return;

            case RFC_PORTNEG_IND:
                PRINT(("RFC_PORTNEG_IND"));
                connectionHandleRfcommPortNegInd((RFC_PORTNEG_IND_T*)message);
                return;

            case RFC_MODEM_STATUS_CFM:
                PRINT(("RFC_MODEM_STATUS_CFM"));
                connectionHandleRfcommControlCfm((RFC_MODEM_STATUS_CFM_T*)message);
                return;

            case RFC_MODEM_STATUS_IND:
                PRINT(("RFC_MODEM_STATUS_IND"));
                connectionHandleRfcommControlInd((RFC_MODEM_STATUS_IND_T*)message);
                return;

            case RFC_LINESTATUS_IND:
                PRINT(("RFC_LINESTATUS_IND"));
                connectionHandleRfcommLineStatusInd((RFC_LINESTATUS_IND_T*)message);
                return;

            case RFC_LINESTATUS_CFM:
                PRINT(("RFC_LINESTATUS_CFM"));
                connectionHandleRfcommLineStatusCfm((RFC_LINESTATUS_CFM_T*)message);
                return;

            /* Ignore these messages */
            case RFC_ERROR_IND:
                PRINT(("RFC_ERROR_IND"));
                return;

            case RFC_NSC_IND:
                PRINT(("RFC_NSC_IND"));
                return;

            /* Necessary to avoid compiler warning for downstream prims. */
            default:
                break;
        }
    }
    else if (theCm->state == connectionInitialising)
    {
        if (message->type == RFC_INIT_CFM)
        {
            PRINT(("RFC_INIT_CFM\n"));
            connectionSendInternalInitCfm(connectionInitRfc);
            return;
        }
    }

    /* Prims we are not handling - for now panic the app */
    handleUnexpected(connectionUnexpectedRfcPrim, theCm->state, message->type);
}
#endif

/****************************************************************************
NAME
    connectionBluestackHandlerReadySdp

DESCRIPTION
    SDP message handler for the ready state.

RETURNS
    void
*/
#ifndef CL_EXCLUDE_SDP
static void connectionBluestackHandlerSdp(connectionState *theCm, SDS_UPRIM_T *message)
{
    if (theCm->state == connectionReady)
    {
        switch (message->type)
        {
            case SDS_REGISTER_CFM:
                PRINT(("SDS_REGISTER_CFM\n"));
                connectionHandleSdpRegisterCfm(&theCm->sdpState, (SDS_REGISTER_CFM_T *)message);
                return;

            case SDS_UNREGISTER_CFM:
                PRINT(("SDS_UNREGISTER_CFM\n"));
                connectionHandleSdpUnregisterCfm(&theCm->sdpState, (SDS_UNREGISTER_CFM_T *)message);
                return;

            case SDC_OPEN_SEARCH_CFM:
                PRINT(("SDC_OPEN_SEARCH_CFM\n"));
                connectionHandleSdpOpenSearchCfm(&theCm->sdpState, (SDC_OPEN_SEARCH_CFM_T *)message);
                return;

            case SDC_CLOSE_SEARCH_IND:
                PRINT(("SDC_CLOSE_SEARCH_IND\n"));
                connectionHandleSdpCloseSearchInd(&theCm->sdpState, (SDC_CLOSE_SEARCH_IND_T *)message);
                return;

            case SDC_SERVICE_SEARCH_CFM:
                PRINT(("SDC_SERVICE_SEARCH_CFM\n"));
                connectionHandleSdpServiceSearchCfm(&theCm->sdpState, (SDC_SERVICE_SEARCH_CFM_T *)message);
                return;

            case SDC_SERVICE_ATTRIBUTE_CFM:
                PRINT(("SDC_SERVICE_ATTRIBUTE_CFM\n"));
                connectionHandleSdpAttributeSearchCfm(&theCm->sdpState, (SDC_SERVICE_ATTRIBUTE_CFM_T *)message);
                return;

            case SDC_SERVICE_SEARCH_ATTRIBUTE_CFM:
                PRINT(("SDC_SERVICE_SEARCH_ATTRIBUTE_CFM\n"));
                connectionHandleSdpServiceSearchAttributeCfm(&theCm->sdpState, (SDC_SERVICE_SEARCH_ATTRIBUTE_CFM_T *)message);
                return;
        }
    }

    /* Prims we are not handling - for now panic the app */
    handleUnexpected(connectionUnexpectedSdpPrim, theCm->state, message->type);
}
#endif

/****************************************************************************
NAME
    connectionBluestackHandlerUninitialised

DESCRIPTION
    Message handler for the uninitialised state.

RETURNS
    void
*/
static void connectionBluestackHandlerUninitialised(connectionState *theCm, MessageId id, Message message)
{
    /* Depending upon the message id...*/
    if (id == CL_INTERNAL_INIT_REQ)
    {
        PRINT(("CL_INTERNAL_INIT_REQ\n"));
        connectionHandleInternalInit(connectionInit);
    }
    else
    {
        /* Prims we are not handling - Not panicing the app in DEBUG Mode. Just Print this INFO and ignore it.*/
        CL_DEBUG_INFO(("Ignored Unexpected Message - Code 0x%x State 0x%x MsgId 0x%x\n", connectionUnexpectedCmPrim, theCm->state, id))
    }
}

/****************************************************************************
NAME
    connectionBluestackHandlerInitialising

DESCRIPTION
    Message handler for the initialising state.

RETURNS
    void
*/
static void connectionBluestackHandlerInitialising(connectionState *theCm, MessageId id, Message message)
{
    /* Depending upon the message id...*/
    switch (id)
    {
        case CL_INTERNAL_INIT_CFM:
            PRINT(("CL_INTERNAL_INIT_CFM\n"));
            connectionHandleInternalInit(((CL_INTERNAL_INIT_CFM_T*)message)->state);
            break;

        case CL_INTERNAL_INIT_TIMEOUT_IND:
            PRINT(("CL_INTERNAL_INIT_TIMEOUT_IND\n"));
            (void)MessageCancelFirst(&theCm->task, CL_INTERNAL_INIT_CFM);
            SET_CM_STATE(connectionUninitialised);
            connectionSendInitCfm(theCm->theAppTask, fail, bluetooth_unknown);
            break;

        case CL_INTERNAL_SM_INIT_REQ:
            PRINT(("CL_INTERNAL_SM_INIT_REQ\n"));
            handleSecurityInitReq(&theCm->infoState, (CL_INTERNAL_SM_INIT_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ:
             PRINT(("CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ\n"));
             connectionHandleReadLocalVersionRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ_T *)message);
             break;

        case CL_INTERNAL_DM_SET_BT_VERSION_REQ:
            PRINT(("CL_INTERNAL_DM_SET_BT_VERSION_REQ\n"));
            connectionHandleSetBtVersionReq(&theCm->infoState, (CL_INTERNAL_DM_SET_BT_VERSION_REQ_T *)message);
            break;

        default:
            /* Prims we are not handling - for now panic the app */
            handleUnexpected(connectionUnhandledMessage, theCm->state, id);
            break;
    }
}

/****************************************************************************
NAME
    connectionBluestackHandlerTestMode

DESCRIPTION
    Message handler for the test-mode state.

RETURNS
    void
*/
static void connectionBluestackHandlerTestMode(connectionState *theCm, MessageId id, Message message)
{
    /* Depending upon the message id...*/
    switch (id)
    {
        case CL_INTERNAL_SM_SET_SC_MODE_REQ:
            PRINT(("CL_INTERNAL_SM_SET_SC_MODE_REQ\n"));
            handleSetSecurityModeReq(&theCm->smState, (CL_INTERNAL_SM_SET_SC_MODE_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ:
            PRINT(("CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ\n"));
            handleSetSspSecurityLevelReq(&theCm->smState, &theCm->infoState, (CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ:
            PRINT(("CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ\n"));
            handleSecModeConfigReq(&theCm->smState, &theCm->infoState, (CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ_T*)message);
            break;

        case CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ\n"));
            connectionHandleWriteScanEnableRequest((CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ:
            PRINT(("CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ\n"));
            connectionHandleReadLocalVersionRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_SET_BT_VERSION_REQ:
            PRINT(("CL_INTERNAL_DM_SET_BT_VERSION_REQ\n"));
            connectionHandleSetBtVersionReq(&theCm->infoState, (CL_INTERNAL_DM_SET_BT_VERSION_REQ_T*)message);
            break;

        case CL_INTERNAL_DM_DUT_REQ:
            /* Once the device has entered DUT mode successfully make sure the connection library rejects
             * all subsequent requests to enter DUT and sends back a confirm message to the client.
             */
            PRINT(("CL_INTERNAL_DM_DUT_REQ\n"));
            connectionSendDutCfmToClient(theCm->theAppTask, fail);
            break;

        default:
            /* Prims we are not handling - for now panic the app */
            handleUnexpected(connectionUnhandledMessage, theCm->state, id);
            break;
    }
}

/****************************************************************************
NAME
    connectionBluestackHandlerReady

DESCRIPTION
    Message handler for the ready state.

RETURNS
    void
*/
static void connectionBluestackHandlerReady(connectionState *theCm, MessageId id, Message message)
{
    /* Depending upon the message id...*/
    switch(id)
    {
        case CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ:
            PRINT(("CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ\n"));
            handleSecModeConfigReq(&theCm->smState, &theCm->infoState, (CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ:
            PRINT(("CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ\n"));
            handleSetSspSecurityLevelReq(&theCm->smState, &theCm->infoState, (CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_READ_LOCAL_OOB_DATA_REQ:
            PRINT(("CL_INTERNAL_SM_READ_LOCAL_OOB_DATA_REQ\n"));
            connectionHandleReadLocalOobDataReq(&theCm->infoState, &theCm->smState, (CL_INTERNAL_SM_READ_LOCAL_OOB_DATA_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_AUTHENTICATION_REQ:
            PRINT(("CL_INTERNAL_SM_AUTHENTICATION_REQ\n"));
            connectionHandleAuthenticationReq(&theCm->smState, (CL_INTERNAL_SM_AUTHENTICATION_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_CANCEL_AUTHENTICATION_REQ:
            PRINT(("CL_INTERNAL_SM_CANCEL_AUTHENTICATION_REQ\n"));
            connectionHandleCancelAuthenticationReq(&theCm->infoState, &theCm->smState, (CL_INTERNAL_SM_CANCEL_AUTHENTICATION_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_AUTHENTICATION_TIMEOUT_IND:
            PRINT(("CL_INTERNAL_SM_AUTHENTICATION_TIMEOUT_IND\n"));
            connectionHandleAuthenticationTimeout(&theCm->smState);
            break;

        case CL_INTERNAL_SM_REGISTER_REQ:
            PRINT(("CL_INTERNAL_SM_REGISTER_REQ\n"));
            handleRegisterReq((CL_INTERNAL_SM_REGISTER_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_UNREGISTER_REQ:
            PRINT(("CL_INTERNAL_SM_UNREGISTER_REQ\n"));
            handleUnRegisterReq((CL_INTERNAL_SM_UNREGISTER_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_REGISTER_OUTGOING_REQ:
            PRINT(("CL_INTERNAL_SM_REGISTER_OUTGOING_REQ\n"));
            handleRegisterOutgoingReq((CL_INTERNAL_SM_REGISTER_OUTGOING_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_UNREGISTER_OUTGOING_REQ:
            PRINT(("CL_INTERNAL_SM_UNREGISTER_OUTGOING_REQ\n"));
            handleUnRegisterOutgoingReq((CL_INTERNAL_SM_UNREGISTER_OUTGOING_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_ENCRYPT_REQ:
            PRINT(("CL_INTERNAL_SM_ENCRYPT_REQ\n"));
            handleEncryptReq(&theCm->smState, (CL_INTERNAL_SM_ENCRYPT_REQ_T *)message);
            break;

        case CL_INTERNAL_SM_ENCRYPTION_KEY_REFRESH_REQ:
            PRINT(("CL_INTERNAL_SM_ENCRYPTION_KEY_REFRESH_REQ\n"));
            handleEncryptionKeyRefreshReq(&theCm->infoState, (CL_INTERNAL_SM_ENCRYPTION_KEY_REFRESH_REQ_T *)message);
            break;

        case CL_INTERNAL_SM_PIN_REQUEST_RES:
            PRINT(("CL_INTERNAL_SM_PIN_REQUEST_RES\n"));
            handlePinRequestRes((CL_INTERNAL_SM_PIN_REQUEST_RES_T*)message);
            break;

        case CL_INTERNAL_SM_IO_CAPABILITY_REQUEST_RES:
            PRINT(("CL_INTERNAL_SM_IO_CAPABILITY_REQUEST_RES\n"));
            handleIoCapabilityRequestRes(&theCm->smState, (CL_INTERNAL_SM_IO_CAPABILITY_REQUEST_RES_T*)message);
            break;

        case CL_INTERNAL_SM_USER_CONFIRMATION_REQUEST_RES:
            PRINT(("CL_INTERNAL_SM_USER_CONFIRMATION_REQUEST_RES\n"));
            handleUserConfirmationRequestRes((CL_INTERNAL_SM_USER_CONFIRMATION_REQUEST_RES_T*)message);
            break;

        case CL_INTERNAL_SM_USER_PASSKEY_REQUEST_RES:
            PRINT(("CL_INTERNAL_SM_USER_PASSKEY_REQUEST_RES\n"));
            handleUserPasskeyRequestRes((CL_INTERNAL_SM_USER_PASSKEY_REQUEST_RES_T*)message);
            break;

        case CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ:
            PRINT(("CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ\n"));
            handleSendKeypressNotificationReq((CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_SET_TRUST_LEVEL_REQ:
            PRINT(("CL_INTERNAL_SM_SET_TRUST_LEVEL_REQ\n"));
            handleSetTrustLevelReq(&theCm->smState, (CL_INTERNAL_SM_SET_TRUST_LEVEL_REQ_T*)message);
            break;

        case CL_INTERNAL_SM_AUTHORISE_RES:
            PRINT(("CL_INTERNAL_SM_AUTHORISE_RES\n"));
            handleAuthoriseRes((CL_INTERNAL_SM_AUTHORISE_RES_T*)message);
            break;

        case CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ:
            PRINT(("CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ\n"));
            handleAddAuthDeviceReq(&theCm->smState, (CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ_T *)message);
            break;

        case CL_INTERNAL_SM_GET_AUTH_DEVICE_REQ:
            PRINT(("CL_INTERNAL_SM_GET_AUTH_DEVICE_REQ\n"));
            handleGetAuthDeviceReq(&theCm->smState, (CL_INTERNAL_SM_GET_AUTH_DEVICE_REQ_T *)message);
            break;

        case CL_INTERNAL_SM_SET_SC_MODE_REQ:
            PRINT(("CL_INTERNAL_SM_SET_SC_MODE_REQ\n"));
            handleSetSecurityModeReq(&theCm->smState, (CL_INTERNAL_SM_SET_SC_MODE_REQ_T*)message);
            break;

        case CL_INTERNAL_DM_DUT_REQ:
            PRINT(("CL_INTERNAL_DM_DUT_REQ\n"));
            SET_CM_STATE(connectionTestMode);
            connectionHandleEnterDutModeReq(&theCm->infoState);
            break;

        case CL_INTERNAL_DM_SET_BT_VERSION_REQ:
            PRINT(("CL_INTERNAL_DM_SET_BT_VERSION_REQ\n"));
            connectionHandleSetBtVersionReq(&theCm->infoState, (CL_INTERNAL_DM_SET_BT_VERSION_REQ_T*)message);
            break;

        case CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ:
            PRINT(("CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ\n"));
            connectionHandleReadLocalVersionRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ\n"));
            connectionHandleWriteInquiryScanTypeRequest(
                (CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ\n"));
            connectionHandleWritePageScanTypeRequest(
                (CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ\n"));
            connectionHandleWriteScanEnableRequest((CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ_T *)message);
            break;

#ifndef CL_EXCLUDE_INQUIRY
        case CL_INTERNAL_DM_INQUIRY_REQ:
            PRINT(("CL_INTERNAL_DM_INQUIRY_REQ\n"));
            connectionHandleInquiryStart(&theCm->inqState, (CL_INTERNAL_DM_INQUIRY_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_INQUIRY_CANCEL_REQ:
            PRINT(("CL_INTERNAL_DM_INQUIRY_CANCEL_REQ\n"));
            connectionHandleInquiryCancel(&theCm->inqState, (CL_INTERNAL_DM_INQUIRY_CANCEL_REQ_T *)message);
            break;
#endif

        case CL_INTERNAL_DM_READ_REMOTE_NAME_REQ:
            PRINT(("CL_INTERNAL_DM_READ_REMOTE_NAME_REQ\n"));
            connectionHandleReadRemoteName(&theCm->inqState,
                         (CL_INTERNAL_DM_READ_REMOTE_NAME_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_REMOTE_NAME_CANCEL_REQ:
            PRINT(("CL_INTERNAL_DM_READ_REMOTE_NAME_CANCEL_REQ\n"));
            connectionHandleReadRemoteNameCancel(&theCm->inqState,
                      (CL_INTERNAL_DM_READ_REMOTE_NAME_CANCEL_REQ_T *)message);
            break;


        case CL_INTERNAL_DM_READ_LOCAL_NAME_REQ:
            PRINT(("CL_INTERNAL_DM_READ_LOCAL_NAME_REQ\n"));
            connectionHandleReadLocalName(&theCm->inqState, (CL_INTERNAL_DM_READ_LOCAL_NAME_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_CHANGE_LOCAL_NAME_REQ:
            PRINT(("CL_INTERNAL_DM_CHANGE_LOCAL_NAME_REQ\n"));
            connectionHandleChangeLocalName((CL_INTERNAL_DM_CHANGE_LOCAL_NAME_REQ_T *)message);
            break;

#ifndef CL_EXCLUDE_INQUIRY
        case CL_INTERNAL_DM_WRITE_INQUIRY_TX_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_INQUIRY_TX_REQ\n"));
            connectionHandleWriteInquiryTx(&theCm->infoState, &theCm->inqState, (CL_INTERNAL_DM_WRITE_INQUIRY_TX_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_INQUIRY_TX_REQ:
            PRINT(("CL_INTERNAL_DM_READ_INQUIRY_TX_REQ\n"));
            connectionHandleReadInquiryTx(&theCm->infoState, &theCm->inqState, (CL_INTERNAL_DM_READ_INQUIRY_TX_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_TX_POWER_REQ:
             connectionHandleReadTxPower(&theCm->readTxPwrState, 
                                (CL_INTERNAL_DM_READ_TX_POWER_REQ_T *)message);
             break;

        case CL_INTERNAL_DM_WRITE_EIR_DATA_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_EIR_DATA_REQ\n"));
            connectionHandleWriteEirDataRequest(&theCm->infoState, (CL_INTERNAL_DM_WRITE_EIR_DATA_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_EIR_DATA_REQ:
            PRINT(("CL_INTERNAL_DM_READ_EIR_DATA_REQ\n"));
            connectionHandleReadEirDataRequest(&theCm->infoState, &theCm->inqState, (CL_INTERNAL_DM_READ_EIR_DATA_REQ_T *)message);
            break;
#endif

        case CL_INTERNAL_DM_READ_CLASS_OF_DEVICE_REQ:
            PRINT(("CL_INTERNAL_DM_READ_CLASS_OF_DEVICE_REQ\n"));
            connectionHandleReadClassOfDeviceRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_CLASS_OF_DEVICE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_PAGESCAN_ACTIVITY_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_PAGESCAN_ACTIVITY_REQ\n"));
            connectionHandleWritePageScanActivityRequest((CL_INTERNAL_DM_WRITE_PAGESCAN_ACTIVITY_REQ_T *)message);
            break;

#ifndef CL_EXCLUDE_INQUIRY
        case CL_INTERNAL_DM_WRITE_INQSCAN_ACTIVITY_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_INQSCAN_ACTIVITY_REQ\n"));
            connectionHandleWriteInquiryScanActivityRequest((CL_INTERNAL_DM_WRITE_INQSCAN_ACTIVITY_REQ_T *)message);
            break;
#endif

        case CL_INTERNAL_DM_WRITE_CLASS_OF_DEVICE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_CLASS_OF_DEVICE_REQ\n"));
            connectionHandleWriteCodRequest((CL_INTERNAL_DM_WRITE_CLASS_OF_DEVICE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_CACHED_PAGE_MODE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_CACHED_PAGE_MODE_REQ\n"));
            connectionHandleWriteCachedPageModeRequest((CL_INTERNAL_DM_WRITE_CACHED_PAGE_MODE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_CACHED_CLK_OFFSET_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_CACHED_CLK_OFFSET_REQ\n"));
            connectionHandleWriteCachedClkOffsetRequest((CL_INTERNAL_DM_WRITE_CACHED_CLK_OFFSET_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_CLEAR_PARAM_CACHE_REQ:
            PRINT(("CL_INTERNAL_DM_CLEAR_PARAM_CACHE_REQ\n"));
            connectionHandleClearParamCacheRequest((CL_INTERNAL_DM_CLEAR_PARAM_CACHE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_FLUSH_TIMEOUT_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_FLUSH_TIMEOUT_REQ\n"));
            connectionHandleWriteFlushTimeoutRequest((CL_INTERNAL_DM_WRITE_FLUSH_TIMEOUT_REQ_T *)message);
            break;

#ifndef CL_EXCLUDE_INQUIRY
        case CL_INTERNAL_DM_WRITE_IAC_LAP_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_IAC_LAP_REQ\n"));
            connectionHandleWriteIacLapRequest(&theCm->inqState, (CL_INTERNAL_DM_WRITE_IAC_LAP_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_WRITE_INQUIRY_MODE_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_INQUIRY_MODE_REQ\n"));
            connectionHandleWriteInquiryModeRequest(&theCm->inqState, (CL_INTERNAL_DM_WRITE_INQUIRY_MODE_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_INQUIRY_MODE_REQ:
            PRINT(("CL_INTERNAL_DM_READ_INQUIRY_MODE_REQ\n"));
            connectionHandleReadInquiryModeRequest(&theCm->inqState, (CL_INTERNAL_DM_READ_INQUIRY_MODE_REQ_T *)message);
            break;
#endif

        case CL_INTERNAL_DM_READ_BD_ADDR_REQ:
            PRINT(("CL_INTERNAL_DM_READ_BD_ADDR_REQ\n"));
            connectionHandleReadAddrRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_BD_ADDR_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_LINK_QUALITY_REQ:
            PRINT(("CL_INTERNAL_DM_READ_LINK_QUALITY_REQ\n"));
            connectionHandleReadLinkQualityRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_LINK_QUALITY_REQ_T *)message);
            break;

        case CL_INTERNAL_DM_READ_RSSI_REQ:
            PRINT(("CL_INTERNAL_DM_READ_RSSI_REQ\n"));
            connectionHandleReadRssiRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_RSSI_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_READ_CLK_OFFSET_REQ:
            PRINT(("CL_INTERNAL_DM_READ_CLK_OFFSET_REQ\n"));
            connectionHandleReadclkOffsetRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_CLK_OFFSET_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_READ_REMOTE_SUPP_FEAT_REQ:
            PRINT(("CL_INTERNAL_DM_READ_REMOTE_SUPP_FEAT_REQ\n"));
            connectionHandleReadRemoteSupportedFeaturesRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_REMOTE_SUPP_FEAT_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_ACL_FORCE_DETACH_REQ:
            PRINT(("CL_INTERNAL_DM_ACL_FORCE_DETACH_REQ\n"));
            connectionDmHandleAclDetach((CL_INTERNAL_DM_ACL_FORCE_DETACH_REQ_T*)message);
            break;

         case CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ:
             PRINT(("CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ\n"));
             connectionHandleReadRemoteVersionRequest(&theCm->infoState, (CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ_T *) message);
             break;

        case CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ:
             PRINT(("CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ\n"));
            connectionHandleSmChangeLinkKeyReq((CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ_T*) message);
            break;

#ifndef CL_EXCLUDE_SDP
        case CL_INTERNAL_SDP_REGISTER_RECORD_REQ:
            PRINT(("CL_INTERNAL_SDP_REGISTER_RECORD_REQ\n"));
            connectionHandleSdpRegisterRequest(&theCm->sdpState, (CL_INTERNAL_SDP_REGISTER_RECORD_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_UNREGISTER_RECORD_REQ:
            PRINT(("CL_INTERNAL_SDP_UNREGISTER_RECORD_REQ\n"));
            connectionHandleSdpUnregisterRequest(&theCm->sdpState, (CL_INTERNAL_SDP_UNREGISTER_RECORD_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_CONFIG_SERVER_MTU_REQ:
            PRINT(("CL_INTERNAL_SDP_CONFIG_SERVER_MTU_REQ\n"));
            connectionHandleSdpServerConfigMtu(&theCm->sdpState, (CL_INTERNAL_SDP_CONFIG_SERVER_MTU_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_CONFIG_CLIENT_MTU_REQ:
            PRINT(("CL_INTERNAL_SDP_CONFIG_CLIENT_MTU_REQ\n"));
            connectionHandleSdpClientConfigMtu(&theCm->sdpState, (CL_INTERNAL_SDP_CONFIG_CLIENT_MTU_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_OPEN_SEARCH_REQ:
            PRINT(("CL_INTERNAL_SDP_OPEN_SEARCH_REQ\n"));
            connectionHandleSdpOpenSearchRequest(&theCm->sdpState, (CL_INTERNAL_SDP_OPEN_SEARCH_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_CLOSE_SEARCH_REQ:
            PRINT(("CL_INTERNAL_SDP_CLOSE_SEARCH_REQ\n"));
            connectionHandleSdpCloseSearchRequest(&theCm->sdpState, (CL_INTERNAL_SDP_CLOSE_SEARCH_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_SERVICE_SEARCH_REQ:
            PRINT(("CL_INTERNAL_SDP_SERVICE_SEARCH_REQ\n"));
            connectionHandleSdpServiceSearchRequest(&theCm->sdpState, (CL_INTERNAL_SDP_SERVICE_SEARCH_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_ATTRIBUTE_SEARCH_REQ:
            PRINT(("CL_INTERNAL_SDP_ATTRIBUTE_SEARCH_REQ\n"));
            connectionHandleSdpAttributeSearchRequest(&theCm->sdpState, (CL_INTERNAL_SDP_ATTRIBUTE_SEARCH_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_SERVICE_SEARCH_ATTRIBUTE_REQ:
            PRINT(("CL_INTERNAL_SDP_SERVICE_SEARCH_ATTRIBUTE_REQ\n"));
            connectionHandleSdpServiceSearchAttrRequest(&theCm->sdpState, (CL_INTERNAL_SDP_SERVICE_SEARCH_ATTRIBUTE_REQ_T *)message);
            break;

        case CL_INTERNAL_SDP_TERMINATE_PRIMITIVE_REQ:
            PRINT(("CL_INTERNAL_SDP_TERMINATE_PRIMITIVE_REQ\n"));
            connectionHandleSdpTerminatePrimitiveRequest(&theCm->sdpState, (CL_INTERNAL_SDP_TERMINATE_PRIMITIVE_REQ_T *)message);
            break;
#endif

#ifndef CL_EXCLUDE_L2CAP
        case CL_INTERNAL_L2CAP_REGISTER_REQ:
            PRINT(("CL_INTERNAL_L2CAP_REGISTER_REQ\n"));
            connectionHandleL2capRegisterReq((CL_INTERNAL_L2CAP_REGISTER_REQ_T *)message);
            break;

        case CL_INTERNAL_L2CAP_UNREGISTER_REQ:
            PRINT(("CL_INTERNAL_L2CAP_UNREGISTER_REQ\n"));
            connectionHandleL2capUnregisterReq((CL_INTERNAL_L2CAP_UNREGISTER_REQ_T *)message);
            break;

        case CL_INTERNAL_L2CAP_CONNECT_REQ:
            PRINT(("CL_INTERNAL_L2CAP_CONNECT_REQ\n"));
            connectionHandleL2capConnectReq((CL_INTERNAL_L2CAP_CONNECT_REQ_T *)message);
            break;

        case CL_INTERNAL_L2CAP_CONNECT_RES:
            PRINT(("CL_INTERNAL_L2CAP_CONNECT_RES\n"));
            connectionHandleL2capConnectRes((CL_INTERNAL_L2CAP_CONNECT_RES_T *) message);
            break;

        case CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_REQ:
            PRINT(("CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_REQ\n"));
            connectionHandleL2capMapConnectionlessReq(&theCm->l2capState, (CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_REQ_T *) message);
            break;

        case CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_RES:
            PRINT(("CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_RES\n"));
            connectionHandleL2capMapConnectionlessRsp((CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_RES_T *) message);
            break;

        case CL_INTERNAL_L2CAP_UNMAP_CONNECTIONLESS_REQ:
            PRINT(("CL_INTERNAL_L2CAP_UNMAP_CONNECTIONLESS_REQ\n"));
            connectionHandleL2capUnmapConnectionlessReq((CL_INTERNAL_L2CAP_UNMAP_CONNECTIONLESS_REQ_T *)message);
            break;

        case CL_INTERNAL_L2CAP_DISCONNECT_REQ:
            PRINT(("CL_INTERNAL_L2CAP_DISCONNECT_REQ\n"));
            connectionHandleL2capDisconnectReq((CL_INTERNAL_L2CAP_DISCONNECT_REQ_T *) message);
            break;

        case CL_INTERNAL_L2CAP_DISCONNECT_RSP:
            PRINT(("CL_INTERNAL_L2CAP_DISCONNECT_RSP"));
            connectionHandleL2capDisconnectRes((CL_INTERNAL_L2CAP_DISCONNECT_RSP_T*)message);
            break;

#endif /* !CL_EXCLUDE_L2CAP */

#ifndef CL_EXCLUDE_RFCOMM
        case CL_INTERNAL_RFCOMM_REGISTER_REQ:
            PRINT(("CL_INTERNAL_RFCOMM_REGISTER_REQ\n"));
            connectionHandleRfcommRegisterReq((CL_INTERNAL_RFCOMM_REGISTER_REQ_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_UNREGISTER_REQ:
            PRINT(("CL_INTERNAL_UNRFCOMM_REGISTER_REQ\n"));
            connectionHandleRfcommUnregisterReq((CL_INTERNAL_RFCOMM_UNREGISTER_REQ_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_CONNECT_REQ:
            PRINT(("CL_INTERNAL_RFCOMM_CONNECT_REQ\n"));
            connectionHandleRfcommConnectReq((CL_INTERNAL_RFCOMM_CONNECT_REQ_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_CONNECT_RES:
            PRINT(("CL_INTERNAL_RFCOMM_CONNECT_RES\n"));
            connectionHandleRfcommConnectRes((CL_INTERNAL_RFCOMM_CONNECT_RES_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_DISCONNECT_REQ:
            PRINT(("CL_INTERNAL_RFCOMM_DISCONNECT_REQ\n"));
            connectionHandleRfcommDisconnectReq((CL_INTERNAL_RFCOMM_DISCONNECT_REQ_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_DISCONNECT_RSP:
            PRINT(("CL_INTERNAL_RFCOMM_DISCONNECT_RSP\n"));
            connectionHandleRfcommDisconnectRsp((CL_INTERNAL_RFCOMM_DISCONNECT_RSP_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_PORTNEG_REQ:
            PRINT(("CL_INTERNAL_RFCOMM_PORTNEG_REQ"));
            connectionHandleRfcommPortNegReq((CL_INTERNAL_RFCOMM_PORTNEG_REQ_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_PORTNEG_RSP:
            PRINT(("CL_INTERNAL_RFCOMM_PORT_NEG_RSP"));
            connectionHandleRfcommPortNegRsp((CL_INTERNAL_RFCOMM_PORTNEG_RSP_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_CONTROL_REQ:
            PRINT(("CL_INTERNAL_RFCOMM_CONTROL_REQ\n"));
            connectionHandleRfcommControlReq((CL_INTERNAL_RFCOMM_CONTROL_REQ_T *)message);
            break;

        case CL_INTERNAL_RFCOMM_LINE_STATUS_REQ:
            PRINT(("CL_INTERNAL_RFCOMM_LINE_STATUS_REQ\n"));
            connectionHandleRfcommLineStatusReq((CL_INTERNAL_RFCOMM_LINE_STATUS_REQ_T*)message);
            break;
#endif

#ifndef CL_EXCLUDE_SYNC
        case CL_INTERNAL_SYNC_REGISTER_REQ:
            PRINT(("CL_INTERNAL_SYNC_REGISTER_REQ\n"));
            connectionHandleSyncRegisterReq((CL_INTERNAL_SYNC_REGISTER_REQ_T *) message);
            break;

        case CL_INTERNAL_SYNC_UNREGISTER_REQ:
            PRINT(("CL_INTERNAL_SYNC_UNREGISTER_REQ\n"));
            connectionHandleSyncUnregisterReq((CL_INTERNAL_SYNC_UNREGISTER_REQ_T *) message);
            break;

        case CL_INTERNAL_SYNC_CONNECT_REQ:
            PRINT(("CL_INTERNAL_SYNC_CONNECT_REQ\n"));
            connectionHandleSyncConnectReq((CL_INTERNAL_SYNC_CONNECT_REQ_T *) message);
            break;

        case CL_INTERNAL_SYNC_CONNECT_RES:
            PRINT(("CL_INTERNAL_SYNC_CONNECT_RES\n"));
            connectionHandleSyncConnectRes((CL_INTERNAL_SYNC_CONNECT_RES_T *) message);
            break;

        case CL_INTERNAL_SYNC_DISCONNECT_REQ:
            PRINT(("CL_INTERNAL_SYNC_DISCONNECT_REQ\n"));
            connectionHandleSyncDisconnectReq((CL_INTERNAL_SYNC_DISCONNECT_REQ_T *) message);
            break;

        case CL_INTERNAL_SYNC_RENEGOTIATE_REQ:
            PRINT(("CL_INTERNAL_SYNC_RENEGOTIATE_REQ\n"));
            connectionHandleSyncRenegotiateReq((CL_INTERNAL_SYNC_RENEGOTIATE_REQ_T *) message);
            break;

        case CL_INTERNAL_SYNC_REGISTER_TIMEOUT_IND:
            PRINT(("CL_INTERNAL_SYNC_REGISTER_TIMEOUT_IND\n"));
            connectionHandleSyncRegisterTimeoutInd((CL_INTERNAL_SYNC_REGISTER_TIMEOUT_IND_T *) message);
            break;

        case CL_INTERNAL_SYNC_UNREGISTER_TIMEOUT_IND:
            PRINT(("CL_INTERNAL_SYNC_UNREGISTER_TIMEOUT_IND\n"));
            connectionHandleSyncUnregisterTimeoutInd((CL_INTERNAL_SYNC_UNREGISTER_TIMEOUT_IND_T *) message);
            break;
#endif

        case CL_INTERNAL_DM_SET_ROLE_REQ:
            PRINT(("CL_INTERNAL_DM_SET_ROLE_REQ\n"));
            connectionHandleLinkPolicySetRoleReq(&theCm->linkPolicyState, (CL_INTERNAL_DM_SET_ROLE_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_GET_ROLE_REQ:
            PRINT(("CL_INTERNAL_DM_GET_ROLE_REQ\n"));
            connectionHandleLinkPolicyGetRoleReq(&theCm->linkPolicyState, (CL_INTERNAL_DM_GET_ROLE_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_SET_LINK_SUPERVISION_TIMEOUT_REQ:
            PRINT(("CL_INTERNAL_DM_SET_LINK_SUPERVISION_TIMEOUT_REQ\n"));
            connectionHandleSetLinkSupervisionTimeoutReq((CL_INTERNAL_DM_SET_LINK_SUPERVISION_TIMEOUT_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_SET_LINK_POLICY_REQ:
            PRINT(("CL_INTERNAL_DM_SET_LINK_POLICY_REQ\n"));
            connectionHandleSetLinkPolicyReq((CL_INTERNAL_DM_SET_LINK_POLICY_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_SET_SNIFF_SUB_RATE_POLICY_REQ:
            PRINT(("CL_INTERNAL_DM_SET_SNIFF_SUB_RATE_POLICY_REQ\n"));
            connectionHandleSetSniffSubRatePolicyReq(&theCm->infoState, (CL_INTERNAL_DM_SET_SNIFF_SUB_RATE_POLICY_REQ_T *) message);
            break;

        case CL_INTERNAL_DM_WRITE_PAGE_TIMEOUT_REQ:
            PRINT(("CL_INTERNAL_DM_WRITE_PAGE_TIMEOUT_REQ\n"));
            connectionHandleWritePageTimeout((CL_INTERNAL_DM_WRITE_PAGE_TIMEOUT_REQ_T *)message);
        break;

        case CL_INTERNAL_SM_GET_ATTRIBUTE_REQ:
            PRINT(("CL_INTERNAL_SM_GET_ATTRIBUTE_REQ\n"));
            connectionSmHandleGetAttributeReq(theCm->theAppTask, (CL_INTERNAL_SM_GET_ATTRIBUTE_REQ_T *) message);
            break;

        case CL_INTERNAL_SM_GET_INDEXED_ATTRIBUTE_REQ:
            PRINT(("CL_INTERNAL_SM_GET_INDEXED_ATTRIBUTE_REQ\n"));
            connectionSmHandleGetIndexedAttributeReq(theCm->theAppTask, (CL_INTERNAL_SM_GET_INDEXED_ATTRIBUTE_REQ_T *) message);
            break;

#ifndef DISABLE_BLE
        case CL_INTERNAL_SM_DM_SECURITY_REQ:
            PRINT(("CL_INTERNAL_SM_DM_SECURITY_REQ\n"));
            connectionHandleInternalBleDmSecurityReq(
                (CL_INTERNAL_SM_DM_SECURITY_REQ_T *)message
                );
            break;

        case CL_INTERNAL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_REQ:
            PRINT(
                ("CL_INTERNAL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_REQ")
                );
            connectionHandleDmBleConnectionParametersUpdateReq(
                &theCm->l2capState,
                (CL_INTERNAL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_REQ_T*)message);
            break;
#endif /* !DISABLE_BLE */

        default:
            /* Prims we are not handling - for now panic the app */
            handleUnexpected(connectionUnhandledMessage, theCm->state, id);
    }
}

/****************************************************************************
NAME
    connectionBluestackHandler

DESCRIPTION
    This is the main task handler for all messages sent to the Connection
    Library task.

RETURNS
    void
*/
void connectionBluestackHandler(Task task, MessageId id, Message message)
{
    /* Get access to the Connection Library instance state */
    connectionState *theCm = (connectionState *)task;
    connectionStates state = theCm->state;

    PRINT(("connectionBluestackHandler - Id = 0x%x\n",id));

    /* Handle Bluestack primitives seperately */
    switch (id)
    {
        case MESSAGE_BLUESTACK_DM_PRIM:
            connectionBluestackHandlerDm(theCm, (DM_UPRIM_T *)message);
            break;

#ifndef CL_EXCLUDE_RFCOMM
        case MESSAGE_BLUESTACK_RFCOMM_PRIM:
            connectionBluestackHandlerRfcomm(theCm, (RFCOMM_UPRIM_T *)message);
            break;
#endif

#if !defined(CL_EXCLUDE_L2CAP) || !defined(DISABLE_BLE)
        case MESSAGE_BLUESTACK_L2CAP_PRIM:
            connectionBluestackHandlerL2cap(theCm, (L2CA_UPRIM_T *)message);
            break;
#endif

#ifndef CL_EXCLUDE_SDP
        case MESSAGE_BLUESTACK_SDP_PRIM:
            connectionBluestackHandlerSdp(theCm, (SDS_UPRIM_T *)message);
            break;
#endif

        case MESSAGE_BLUESTACK_UDP_PRIM:
        case MESSAGE_BLUESTACK_TCP_PRIM:
            handleUnexpected(connectionUnhandledMessage, theCm->state, id);
            break;

#ifndef CL_EXCLUDE_SDP
        /* CL_SDP_CLOSE_SEARCH_CFM Primitive arrived as a result of an internal
           call to close SDP search, can't avoid so ignore
           Handled as a special case to allow the compiler to generate better
           code for the previous switch statements. */
        case CL_SDP_CLOSE_SEARCH_CFM:
            break;
#endif

        /* Everything else must be internal connection library primitives */
        default:
        {
            switch (state)
            {
                case connectionReady:
                    connectionBluestackHandlerReady(theCm, id, message);
                    break;

                case connectionUninitialised:
                    connectionBluestackHandlerUninitialised(theCm, id, message);
                    break;

                case connectionInitialising:
                    connectionBluestackHandlerInitialising(theCm, id, message);
                    break;

                case connectionTestMode:
                    connectionBluestackHandlerTestMode(theCm, id, message);
                    break;
            }
        }
    }
}

/*lint +e655 +e525 +e830 */
