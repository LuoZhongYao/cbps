/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    connection_private.h
    
DESCRIPTION
    This file contains the type definitions and function prototypes for the 
    Connection Library
*/

#ifndef CONNECTION_PRIVATE_H_
#define CONNECTION_PRIVATE_H_

/* Header files */
#include <stdlib.h>
#include <string.h>

#include <app/message/system_message.h>
#include <message.h>
#include <panic.h>
#include <string.h>

#include "common.h"
#include "init.h"

#ifndef MESSAGE_MAKE
/* 
   Normally picked up from message.h, but might not be present pre
   4.1.1h, so fall back to the old behaviour.
*/
#define MESSAGE_MAKE(N,T) T *N = PanicUnlessNew(T)
#endif

/* Macros used to make messsage primitive creation simpler */
#define MAKE_PRIM_C(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->common.op_code = TYPE; prim->common.length = sizeof(TYPE##_T);
#define MAKE_PRIM_T(TYPE) MESSAGE_MAKE(prim,TYPE##_T); prim->type = TYPE;

#define MAKE_CL_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T);
#define MAKE_CL_MESSAGE_WITH_LEN(TYPE, LEN) TYPE##_T *message = (TYPE##_T *) PanicUnlessMalloc(sizeof(TYPE##_T) + LEN);
#define COPY_CL_MESSAGE(src, dst) *(dst) = *(src);
#define COPY_CL_MESSAGE_WITH_LEN(TYPE, LEN, src, dst) memmove((dst), (src), sizeof(TYPE##_T) + LEN);

#define UNUSED(v) v=v;

/* Macro used to generate debug lib printfs. */
#ifdef CONNECTION_DEBUG_LIB
#include <panic.h>
#include <stdio.h>
#define CL_DEBUG_INFO(x) {printf x;}
#define CL_DEBUG(x)     {printf x;  Panic();}
#define checkStatus(x)    { DM_HCI_STANDARD_COMMAND_CFM_T *cfm = (DM_HCI_STANDARD_COMMAND_CFM_T *) x; if (cfm->status != HCI_SUCCESS) CL_DEBUG(("Ignored result prim reports error: 0x%x\n", cfm->status)); }
#else
#define CL_DEBUG_INFO(x)
#define CL_DEBUG(x)
#define checkStatus(x)
#endif

/* Parameter checking limits for use by debug build */
#ifdef CONNECTION_DEBUG_LIB
/* From the RFCOMM specification TS07.10
 * Server applications registering with an RFCOMM service interface are
 * assigned a Server Channel number in the range 1...30. [0 and 31] should not
 * be used since the corresponding DLCIs are reserved in TS [07.10]
 * It is this value that should be registered in the Service Discovery Database
 */
#define RFCOMM_SERVER_CHANNEL_MIN       (1)
#define RFCOMM_SERVER_CHANNEL_MAX       (30)
#define MIN_AUTHENTICATION_TIMEOUT      (60)
#define MAX_AUTHENTICATION_TIMEOUT      (600)
#define MIN_WRITE_IAC_LAP               (1)
#define MAX_WRITE_IAC_LAP               (4)
#define MIN_TX_POWER                    (-70)
#define MAX_TX_POWER                    (20)
#endif

#define BREDR_KEY_SIZE                  (8)

#define CL_DM_SM_SEC_CONFIG_PENDING      0x01

/* Passed to MessageSend functions if the message being sent does not have a payload defined. */
#define NO_PAYLOAD      (0x0)

/* Max length of the name in change local name or read remote name requests. */
#define  MAX_NAME_LENGTH        (31)

/* As we store the authentication requirements in HCI format we need to define 
   our own unknown bit to use internally */
#define AUTH_REQ_UNKNOWN    (0x08)

/* Connection library global flags */
/* Pass SDP search reasult as a reference */
#define CONNECTION_FLAG_SDP_REFERENCE   0x01
/* Enable Secure Connections at initialization */
#define CONNECTION_FLAG_SC_ENABLE       0x02
/* Unused global flag bit */
#define CONNECTION_FLAG_SCOM_ENABLE     0x04
/* Unused global flag bit */
#define CONNECTION_FLAG_UNUSED_5        0x08
/* Unused global flag bit */
#define CONNECTION_FLAG_UNUSED_4        0x10
/* Unused global flag bit */
#define CONNECTION_FLAG_UNUSED_3        0x20
/* Unused global flag bit */
#define CONNECTION_FLAG_UNUSED_2        0x40
/* Unused global flag bit */
#define CONNECTION_FLAG_UNUSED_1        0x80


/* Connection Library private messages */
#define CM_MSG_BASE     (0x0)

/* Channel map mask */
#define BLE_CHANNEL_MAP_MASK  (0x07)

/* Default Advertising interval */
#define BLE_DEF_ADV_INTERVAL (0x0800)

/* Minimum Advertising interval */
#define BLE_MIN_ADV_INTERVAL (0x00A0)

/*****************************************************************************
 * PSKEYs used by the connection library 
 * PSKEY_CONNLIB0 (100) to PSKEY_CONNLIB49 (149).
 */

/* Attributes related to a device by it's address are persistently stored in 
 * the ConnLib Keys, from 100 to 108. See B-145841.
 */
#define PSKEY_TDL_ATTRIBUTE_BASE                (100)

#define PSKEY_SM_DIV_STATE                      (139)
#define PSKEY_SM_KEY_STATE_IR_ER                (140)

/* The Trusted Device List uses up to 9 PS Keys. 1 index and 8 for storing 
 * the link keys
 */
#define PSKEY_TRUSTED_DEVICE_LIST               (141)
/*
 *
 ****************************************************************************/
enum
{
    /* Initialisation */
    CL_INTERNAL_INIT_TIMEOUT_IND = CM_MSG_BASE,
    CL_INTERNAL_INIT_REQ,
    CL_INTERNAL_INIT_CFM,

    /* Inquiry Entity */
    CL_INTERNAL_DM_INQUIRY_REQ,
    CL_INTERNAL_DM_INQUIRY_CANCEL_REQ,
    CL_INTERNAL_DM_READ_REMOTE_NAME_REQ,
    CL_INTERNAL_DM_READ_REMOTE_NAME_CANCEL_REQ,
    CL_INTERNAL_DM_READ_LOCAL_NAME_REQ,
    CL_INTERNAL_DM_WRITE_INQUIRY_TX_REQ,
    CL_INTERNAL_DM_READ_INQUIRY_TX_REQ,
    CL_INTERNAL_DM_READ_TX_POWER_REQ,
    CL_INTERNAL_DM_WRITE_INQUIRY_MODE_REQ,
    CL_INTERNAL_DM_READ_INQUIRY_MODE_REQ,
    CL_INTERNAL_DM_WRITE_EIR_DATA_REQ,
    CL_INTERNAL_DM_READ_EIR_DATA_REQ,
    CL_INTERNAL_DM_ACL_FORCE_DETACH_REQ,

    /* Security Entity */
    CL_INTERNAL_SM_INIT_REQ,
    CL_INTERNAL_SM_READ_LOCAL_OOB_DATA_REQ,
    CL_INTERNAL_SM_AUTHENTICATION_REQ,
    CL_INTERNAL_SM_CANCEL_AUTHENTICATION_REQ,
    CL_INTERNAL_SM_AUTHENTICATION_TIMEOUT_IND,
    CL_INTERNAL_SM_SET_SC_MODE_REQ,
    CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ,
    CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ,
    CL_INTERNAL_SM_REGISTER_REQ,
    CL_INTERNAL_SM_REGISTER_OUTGOING_REQ,
    CL_INTERNAL_SM_UNREGISTER_REQ,
    CL_INTERNAL_SM_UNREGISTER_OUTGOING_REQ,
    CL_INTERNAL_SM_ENCRYPT_REQ,
    CL_INTERNAL_SM_ENCRYPTION_KEY_REFRESH_REQ,
    CL_INTERNAL_SM_AUTHORISE_RES,
    CL_INTERNAL_SM_PIN_REQUEST_RES,
    CL_INTERNAL_SM_IO_CAPABILITY_REQUEST_RES,
    CL_INTERNAL_SM_USER_CONFIRMATION_REQUEST_RES,
    CL_INTERNAL_SM_USER_PASSKEY_REQUEST_RES,
    CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ,
    CL_INTERNAL_SM_DELETE_AUTH_DEVICE_REQ,
    CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ,
    CL_INTERNAL_SM_GET_AUTH_DEVICE_REQ,
    CL_INTERNAL_SM_SET_TRUST_LEVEL_REQ,

    /* Baseband Entity */
    CL_INTERNAL_DM_READ_CLASS_OF_DEVICE_REQ,
    CL_INTERNAL_DM_WRITE_CLASS_OF_DEVICE_REQ,
    CL_INTERNAL_DM_WRITE_PAGESCAN_ACTIVITY_REQ,
    CL_INTERNAL_DM_WRITE_INQSCAN_ACTIVITY_REQ,
    CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ,
    CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ,
    CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ,
    CL_INTERNAL_DM_WRITE_CACHED_PAGE_MODE_REQ,
    CL_INTERNAL_DM_WRITE_CACHED_CLK_OFFSET_REQ,
    CL_INTERNAL_DM_CLEAR_PARAM_CACHE_REQ,
    CL_INTERNAL_DM_WRITE_FLUSH_TIMEOUT_REQ,
    CL_INTERNAL_DM_CHANGE_LOCAL_NAME_REQ,
    CL_INTERNAL_DM_WRITE_IAC_LAP_REQ,
    CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ,

    /* Informational Entity*/
    CL_INTERNAL_DM_READ_BD_ADDR_REQ,
    CL_INTERNAL_DM_READ_LINK_QUALITY_REQ,
    CL_INTERNAL_DM_READ_RSSI_REQ,
    CL_INTERNAL_DM_READ_CLK_OFFSET_REQ,
    CL_INTERNAL_DM_SET_BT_VERSION_REQ,
    CL_INTERNAL_DM_READ_REMOTE_SUPP_FEAT_REQ,
    CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ,
    CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ,

    /* SDP Entity */
    CL_INTERNAL_SDP_REGISTER_RECORD_REQ,
    CL_INTERNAL_SDP_UNREGISTER_RECORD_REQ,
    CL_INTERNAL_SDP_CONFIG_SERVER_MTU_REQ,
    CL_INTERNAL_SDP_CONFIG_CLIENT_MTU_REQ,
    CL_INTERNAL_SDP_OPEN_SEARCH_REQ,
    CL_INTERNAL_SDP_CLOSE_SEARCH_REQ,
    CL_INTERNAL_SDP_SERVICE_SEARCH_REQ,
    CL_INTERNAL_SDP_ATTRIBUTE_SEARCH_REQ,
    CL_INTERNAL_SDP_SERVICE_SEARCH_ATTRIBUTE_REQ,
    CL_INTERNAL_SDP_TERMINATE_PRIMITIVE_REQ,

    /* L2CAP Connection Management Entity */
    CL_INTERNAL_L2CAP_REGISTER_REQ,
    CL_INTERNAL_L2CAP_UNREGISTER_REQ,
    CL_INTERNAL_L2CAP_CONNECT_REQ,
    CL_INTERNAL_L2CAP_CONNECT_RES,
    CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_REQ,
    CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_RES,
    CL_INTERNAL_L2CAP_UNMAP_CONNECTIONLESS_REQ,
    CL_INTERNAL_L2CAP_DISCONNECT_REQ,
    CL_INTERNAL_L2CAP_DISCONNECT_RSP,

    /* RFCOMM Connection Management Entity */
    CL_INTERNAL_RFCOMM_REGISTER_REQ,
    CL_INTERNAL_RFCOMM_UNREGISTER_REQ,
    CL_INTERNAL_RFCOMM_CONNECT_REQ,
    CL_INTERNAL_RFCOMM_CONNECT_RES,
    CL_INTERNAL_RFCOMM_DISCONNECT_REQ,
    CL_INTERNAL_RFCOMM_DISCONNECT_RSP,
    CL_INTERNAL_RFCOMM_PORTNEG_REQ,
    CL_INTERNAL_RFCOMM_PORTNEG_RSP,
    CL_INTERNAL_RFCOMM_CONTROL_REQ,
    CL_INTERNAL_RFCOMM_LINE_STATUS_REQ,

    /* Synchronous Connection Entity */
    CL_INTERNAL_SYNC_REGISTER_REQ,
    CL_INTERNAL_SYNC_UNREGISTER_REQ,
    CL_INTERNAL_SYNC_CONNECT_REQ,
    CL_INTERNAL_SYNC_CONNECT_RES,
    CL_INTERNAL_SYNC_DISCONNECT_REQ,
    CL_INTERNAL_SYNC_RENEGOTIATE_REQ,
    CL_INTERNAL_SYNC_REGISTER_TIMEOUT_IND,
    CL_INTERNAL_SYNC_UNREGISTER_TIMEOUT_IND,

    /* Link Policy Management Entity */
    CL_INTERNAL_DM_SET_ROLE_REQ,
    CL_INTERNAL_DM_GET_ROLE_REQ,
    CL_INTERNAL_DM_SET_LINK_SUPERVISION_TIMEOUT_REQ,
    CL_INTERNAL_DM_SET_LINK_POLICY_REQ,
    CL_INTERNAL_DM_SET_SNIFF_SUB_RATE_POLICY_REQ,
    CL_INTERNAL_DM_WRITE_PAGE_TIMEOUT_REQ,

    /* DUT support */
    CL_INTERNAL_DM_DUT_REQ,
    
    /* Attribute read from PS */
    CL_INTERNAL_SM_GET_ATTRIBUTE_REQ,
    CL_INTERNAL_SM_GET_INDEXED_ATTRIBUTE_REQ,

    /* BLE */
    CL_INTERNAL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_REQ,
    CL_INTERNAL_SM_DM_SECURITY_REQ
};


typedef struct
{
    connectionInitState   state;
} CL_INTERNAL_INIT_CFM_T;

typedef struct
{
    Task    theAppTask;
    u32 inquiry_lap;
    u8   max_responses;
    u16  timeout;
    u32  class_of_device;
    u16      min_period;
    u16      max_period;
} CL_INTERNAL_DM_INQUIRY_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_DM_INQUIRY_CANCEL_REQ_T;

typedef struct
{
    Task theAppTask;
    bdaddr bd_addr;
} CL_INTERNAL_DM_READ_REMOTE_NAME_REQ_T;

typedef struct
{
    Task theAppTask;
    bdaddr bd_addr;
} CL_INTERNAL_DM_READ_REMOTE_NAME_CANCEL_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_DM_READ_LOCAL_NAME_REQ_T;

typedef struct
{
    Task theAppTask;
    i8 tx_power;
} CL_INTERNAL_DM_WRITE_INQUIRY_TX_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_DM_READ_INQUIRY_TX_REQ_T;

typedef struct
{
    Task theAppTask;
    tp_bdaddr tpaddr;
} CL_INTERNAL_DM_READ_TX_POWER_REQ_T;

typedef struct
{
    Task theAppTask;
    inquiry_mode mode;
} CL_INTERNAL_DM_WRITE_INQUIRY_MODE_REQ_T;

typedef struct
{
    u8 fec_required;
    u8 size_eir_data;
    u8 *eir_data; 
} CL_INTERNAL_DM_WRITE_EIR_DATA_REQ_T;

typedef struct
{
    Task task;
} CL_INTERNAL_DM_READ_EIR_DATA_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_DM_READ_INQUIRY_MODE_REQ_T;

typedef struct
{
    bdaddr bd_addr;
    u16 flags;
    u8 reason;
} CL_INTERNAL_DM_ACL_FORCE_DETACH_REQ_T;

typedef struct
{
    Task                theAppTask;
    u16              options;
    dm_security_mode    security_mode;
    u16              config;
    cl_sm_wae           write_auth_enable;
    encryption_mode     mode3_enc;
} CL_INTERNAL_SM_INIT_REQ_T;

typedef struct
{
    Task task;
    bdaddr bd_addr;
    u32  timeout;
} CL_INTERNAL_SM_AUTHENTICATION_REQ_T;

typedef struct
{
    Task            task;
    TRANSPORT_T     transport;
} CL_INTERNAL_SM_READ_LOCAL_OOB_DATA_REQ_T;

typedef struct
{
    Task task;
    bool force;
} CL_INTERNAL_SM_CANCEL_AUTHENTICATION_REQ_T;

typedef struct
{
    Task theAppTask;
    bdaddr bd_addr;
} CL_INTERNAL_SM_AUTHENTICATION_TIMEOUT_IND_T;

typedef struct
{
    Task                theAppTask;
    dm_security_mode    mode;
    encryption_mode     mode3_enc;
} CL_INTERNAL_SM_SET_SC_MODE_REQ_T;

typedef struct
{
    dm_protocol_id          protocol_id;
    u32                  channel;
    dm_ssp_security_level   ssp_sec_level;
    bool                    outgoing_ok;
    bool                    authorised;
    bool                    disable_legacy;
} CL_INTERNAL_SM_SET_SSP_SECURITY_LEVEL_REQ_T;

typedef struct
{
    Task            theAppTask;
    cl_sm_wae       write_auth_enable;
    bool            debug_keys;
    bool            legacy_auto_pair_key_missing;
} CL_INTERNAL_SM_SEC_MODE_CONFIG_REQ_T;

typedef struct
{
    dm_protocol_id      protocol_id;
    u32              channel;
    bool                outgoing_ok;
    dm_security_in      security_level;
    u16               psm;
} CL_INTERNAL_SM_REGISTER_REQ_T;

typedef struct
{
    dm_protocol_id  protocol_id;
    u32          channel;
} CL_INTERNAL_SM_UNREGISTER_REQ_T;

typedef struct
{
    Task                theAppTask;
    bdaddr              bd_addr;
    dm_protocol_id      protocol_id;
    u32              remote_channel;
    dm_security_out     outgoing_security_level;
} CL_INTERNAL_SM_REGISTER_OUTGOING_REQ_T;

typedef struct
{
    bdaddr           bd_addr;
    dm_protocol_id   protocol_id;
    u32           channel;
} CL_INTERNAL_SM_UNREGISTER_OUTGOING_REQ_T;

typedef struct
{   
    Task    theAppTask;
    Sink    sink;
    bool    encrypt;
} CL_INTERNAL_SM_ENCRYPT_REQ_T;

typedef struct
{   
    typed_bdaddr            taddr;
} CL_INTERNAL_SM_ENCRYPTION_KEY_REFRESH_REQ_T;

typedef struct
{
    typed_bdaddr            taddr;
    u8                   pin_length;
    u8                   pin[HCI_MAX_PIN_LENGTH];
} CL_INTERNAL_SM_PIN_REQUEST_RES_T;

typedef struct
{
    tp_bdaddr               tpaddr;
    cl_sm_io_capability     io_capability;
    bool                    bonding;
    mitm_setting            mitm;
    u16                  key_distribution;
    u8                   oob_data;
    u8*                  oob_hash_c;
    u8*                  oob_rand_r;
} CL_INTERNAL_SM_IO_CAPABILITY_REQUEST_RES_T;

typedef struct
{
    tp_bdaddr           tpaddr;
    bool                confirm;
} CL_INTERNAL_SM_USER_CONFIRMATION_REQUEST_RES_T;

typedef struct
{
    tp_bdaddr           tpaddr;
    bool                cancelled;
    u32              numeric_value;
} CL_INTERNAL_SM_USER_PASSKEY_REQUEST_RES_T;

typedef struct
{
    tp_bdaddr           tpaddr;
    cl_sm_keypress_type type;
} CL_INTERNAL_SM_SEND_KEYPRESS_NOTIFICATION_REQ_T;

typedef struct
{
    Task                theAppTask;
    bdaddr              bd_addr;
    bool                trusted;
} CL_INTERNAL_SM_SET_TRUST_LEVEL_REQ_T;
 
typedef struct
{
    bdaddr           bd_addr;
    dm_protocol_id   protocol_id;
    u32           channel;
    bool             incoming;
    bool             authorised;
} CL_INTERNAL_SM_AUTHORISE_RES_T;

typedef struct
{
    Task                    theAppTask;
    bdaddr                  bd_addr;
    DM_SM_KEY_ENC_BREDR_T   enc_bredr;
    u16                  trusted;
    u16                  bonded;
} CL_INTERNAL_SM_ADD_AUTH_DEVICE_REQ_T;

typedef struct
{
    Task            theAppTask;
    bdaddr          bd_addr;
} CL_INTERNAL_SM_GET_AUTH_DEVICE_REQ_T;

typedef struct 
{
    Task theAppTask;
} CL_INTERNAL_DM_READ_CLASS_OF_DEVICE_REQ_T;

typedef struct
{
    u32 class_of_device;
} CL_INTERNAL_DM_WRITE_CLASS_OF_DEVICE_REQ_T;

typedef struct
{
    u16 ps_interval;
    u16 ps_window;
} CL_INTERNAL_DM_WRITE_PAGESCAN_ACTIVITY_REQ_T;

typedef struct
{
    u16 is_interval;
    u16 is_window;
} CL_INTERNAL_DM_WRITE_INQSCAN_ACTIVITY_REQ_T;

typedef struct
{
    hci_scan_type type;
} CL_INTERNAL_DM_WRITE_INQUIRY_SCAN_TYPE_REQ_T;

typedef struct
{
    hci_scan_type type;
} CL_INTERNAL_DM_WRITE_PAGE_SCAN_TYPE_REQ_T;
typedef struct
{
    hci_scan_enable mode;
} CL_INTERNAL_DM_WRITE_SCAN_ENABLE_REQ_T;

typedef struct
{
    bdaddr              bd_addr;
    page_scan_mode      ps_mode;
    page_scan_rep_mode  ps_rep_mode;
} CL_INTERNAL_DM_WRITE_CACHED_PAGE_MODE_REQ_T;

typedef struct
{
    bdaddr  bd_addr;
    u16  clock_offset;
} CL_INTERNAL_DM_WRITE_CACHED_CLK_OFFSET_REQ_T;

typedef struct
{
    bdaddr  bd_addr;
} CL_INTERNAL_DM_CLEAR_PARAM_CACHE_REQ_T;

typedef struct
{
    Sink    sink;
    u16  flush_timeout;
} CL_INTERNAL_DM_WRITE_FLUSH_TIMEOUT_REQ_T;

typedef struct
{
    u16  length_name;
    u8   *name;
} CL_INTERNAL_DM_CHANGE_LOCAL_NAME_REQ_T;

typedef struct
{
    Task theAppTask;
    u16 num_iac;
    u32 iac[1];
} CL_INTERNAL_DM_WRITE_IAC_LAP_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_DM_READ_BD_ADDR_REQ_T;

typedef struct
{
    Task theAppTask;
    Sink sink;
} CL_INTERNAL_DM_READ_LINK_QUALITY_REQ_T;

typedef struct
{
    Task theAppTask;
    Sink sink;
    tp_bdaddr tpaddr;         /* only used if sink == NULL */
} CL_INTERNAL_DM_READ_RSSI_REQ_T;

typedef struct
{
    Task    theAppTask;
    Sink    sink;
} CL_INTERNAL_DM_READ_CLK_OFFSET_REQ_T;

typedef struct
{
    Task                theAppTask;
    u8   version;
} CL_INTERNAL_DM_SET_BT_VERSION_REQ_T;

typedef struct
{
    Task    theAppTask;
    Sink    sink;
} CL_INTERNAL_DM_READ_REMOTE_SUPP_FEAT_REQ_T;

typedef struct
{
    Task         theAppTask;
    tp_bdaddr    tpaddr;
} CL_INTERNAL_DM_READ_REMOTE_VERSION_REQ_T;

typedef struct
{
    Task    theAppTask;
} CL_INTERNAL_DM_READ_LOCAL_VERSION_REQ_T;

typedef struct
{
    Sink    sink;
} CL_INTERNAL_SM_CHANGE_LINK_KEY_REQ_T;

#ifndef CL_EXCLUDE_SDP
typedef struct
{
    Task    theAppTask;
    u16  record_length;
    u8   *record;
} CL_INTERNAL_SDP_REGISTER_RECORD_REQ_T;

typedef struct
{
    Task    theAppTask;
    u32  service_handle;
} CL_INTERNAL_SDP_UNREGISTER_RECORD_REQ_T;

typedef struct
{
    u16 mtu;
} CL_INTERNAL_SDP_CONFIG_SERVER_MTU_REQ_T;

typedef struct
{
    u16 mtu;
} CL_INTERNAL_SDP_CONFIG_CLIENT_MTU_REQ_T;

typedef struct
{
    Task        theAppTask;
    bdaddr      bd_addr;
} CL_INTERNAL_SDP_OPEN_SEARCH_REQ_T;

typedef struct
{
    Task    theAppTask;
} CL_INTERNAL_SDP_CLOSE_SEARCH_REQ_T;

typedef struct
{
    Task    theAppTask;
    bdaddr  bd_addr;
    u16    max_responses;
    u16    flags;
    u16  length;
    u8     search_pattern[1];
} CL_INTERNAL_SDP_SERVICE_SEARCH_REQ_T;

typedef struct
{
    Task    theAppTask;
    bdaddr  bd_addr;
    u32  service_handle;
    u16  size_attribute_list;
    u16  max_num_attr;
    u16  flags;
    u8   attribute_list[1];
} CL_INTERNAL_SDP_ATTRIBUTE_SEARCH_REQ_T;

typedef struct
{
    Task    theAppTask;
    bdaddr  bd_addr;
    u16  max_num_attributes; 
    u16  size_search_pattern; 
    u16  size_attribute_list; 
    u16  flags;
    u8   search_attr[1];
} CL_INTERNAL_SDP_SERVICE_SEARCH_ATTRIBUTE_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_SDP_TERMINATE_PRIMITIVE_REQ_T;
#endif

#ifndef CL_EXCLUDE_L2CAP
typedef struct
{
    Task                        clientTask;
    u16                      app_psm;
    u16                      flags;
} CL_INTERNAL_L2CAP_REGISTER_REQ_T;

typedef struct
{
    Task    theAppTask;
    u16  app_psm;
} CL_INTERNAL_L2CAP_UNREGISTER_REQ_T;

typedef struct
{
    Task                theAppTask;
    bdaddr          bd_addr;
    u16              psm_local;
    u16              psm_remote;
    u16              length;
    u16              *data;
} CL_INTERNAL_L2CAP_CONNECT_REQ_T;

typedef struct
{
    Task                theAppTask;
    bool                response;
    u8               identifier;
    u16              psm_local;
    u16              connection_id;
    u16              length;
    u16              *data;
} CL_INTERNAL_L2CAP_CONNECT_RES_T;

typedef struct
{
    Task                            theAppTask;
    bdaddr                          bd_addr;
    u16                          psm_local;
    u16                          psm_remote;
    l2cap_connectionless_data_type  type;
} CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_REQ_T;

typedef struct
{
    Task                            theAppTask;
    Source                          source;
    l2cap_connectionless_data_type  type;
} CL_INTERNAL_L2CAP_MAP_CONNECTIONLESS_RES_T;

typedef struct
{
    Sink                            sink;
} CL_INTERNAL_L2CAP_UNMAP_CONNECTIONLESS_REQ_T;  

typedef struct
{
    Task    theAppTask;
    Sink    sink;
} CL_INTERNAL_L2CAP_DISCONNECT_REQ_T;

typedef struct
{
    u8                 identifier;
    Sink                  sink;
} CL_INTERNAL_L2CAP_DISCONNECT_RSP_T;
#endif

#ifndef DISABLE_BLE
typedef struct
{
    Task                theAppTask;
    typed_bdaddr        taddr;
    u16              min_interval;
    u16              max_interval;
    u16              latency;
    u16              timeout;
    u16              min_ce_length;
    u16              max_ce_length;
} CL_INTERNAL_DM_BLE_CONNECTION_PARAMETERS_UPDATE_REQ_T;
#endif

typedef struct
{
    Task                    theAppTask;
    u8                   suggested_server_channel;
} CL_INTERNAL_RFCOMM_REGISTER_REQ_T;

#ifndef CL_EXCLUDE_RFCOMM
typedef struct
{
    Task                        theAppTask;
    u8                   local_server_channel;
} CL_INTERNAL_RFCOMM_UNREGISTER_REQ_T;

typedef struct
{
    Task                    theAppTask;
    bdaddr                  bd_addr;
    u8                   remote_server_channel;
    u16                  security_channel;
    rfcomm_config_params    config;
} CL_INTERNAL_RFCOMM_CONNECT_REQ_T;

typedef struct
{
    Task                    theAppTask;
    bool                    response;
    Sink                    sink;    
    u8                   server_channel;
    rfcomm_config_params    config;

} CL_INTERNAL_RFCOMM_CONNECT_RES_T;

typedef struct
{
    Task                    theAppTask;
    Sink                    sink;
} CL_INTERNAL_RFCOMM_DISCONNECT_REQ_T;

typedef struct
{
    Sink                    sink;
} CL_INTERNAL_RFCOMM_DISCONNECT_RSP_T;

typedef struct
{
    Task    theAppTask;
    Sink        sink;
    bool        request;
    port_par    port_params;
} CL_INTERNAL_RFCOMM_PORTNEG_REQ_T;

typedef struct
{
    Task               theAppTask;
    Sink        sink;
    port_par    port_params;
} CL_INTERNAL_RFCOMM_PORTNEG_RSP_T;

typedef struct
{
    Task                theAppTask;
    Sink                sink;
    u8               break_signal;
    u8               modem_signal;
} CL_INTERNAL_RFCOMM_CONTROL_REQ_T;

typedef struct
{
    Task                        theAppTask;
    Sink                        sink;
    bool                        error;
    rfcomm_line_status_error    lines_status;
} CL_INTERNAL_RFCOMM_LINE_STATUS_REQ_T;

#endif

#ifndef CL_EXCLUDE_SYNC
typedef struct
{
    Task theAppTask;
} CL_INTERNAL_SYNC_REGISTER_REQ_T;

typedef struct
{
    Task theAppTask;
} CL_INTERNAL_SYNC_UNREGISTER_REQ_T;

typedef struct
{
    Task                     theAppTask;
    Sink                     sink;
    sync_config_params       config_params;
} CL_INTERNAL_SYNC_CONNECT_REQ_T;

typedef struct
{
    Task                theAppTask;
    bdaddr              bd_addr;
    bool                response;
    sync_config_params  config_params;
} CL_INTERNAL_SYNC_CONNECT_RES_T;

typedef struct
{
    Sink                     audio_sink;
    hci_status               reason;
} CL_INTERNAL_SYNC_DISCONNECT_REQ_T;

typedef struct
{
    Task                     theAppTask;
    Sink                     audio_sink;
    sync_config_params       config_params;
} CL_INTERNAL_SYNC_RENEGOTIATE_REQ_T;


typedef struct
{
    Task                     theAppTask;
} CL_INTERNAL_SYNC_REGISTER_TIMEOUT_IND_T;

typedef struct
{
    Task                     theAppTask;
} CL_INTERNAL_SYNC_UNREGISTER_TIMEOUT_IND_T;
#endif

typedef struct
{
    Task        theAppTask;
    Sink        sink;
    hci_role    role;   
} CL_INTERNAL_DM_SET_ROLE_REQ_T;

typedef struct
{
    Task        theAppTask;
    Sink        sink;
} CL_INTERNAL_DM_GET_ROLE_REQ_T;

typedef struct
{
    Sink        sink;
    u16      timeout;
} CL_INTERNAL_DM_SET_LINK_SUPERVISION_TIMEOUT_REQ_T;

typedef struct
{
    Sink        sink;
    u16      size_power_table; 
    lp_power_table const *power_table;
} CL_INTERNAL_DM_SET_LINK_POLICY_REQ_T;

typedef struct
{
    Sink        sink;
    u16      max_remote_latency;
    u16      min_remote_timeout;
    u16      min_local_timeout;
} CL_INTERNAL_DM_SET_SNIFF_SUB_RATE_POLICY_REQ_T;

typedef struct
{
    u16 timeout;
} CL_INTERNAL_DM_WRITE_PAGE_TIMEOUT_REQ_T;

typedef struct
{
    u8   addr_type;
    bdaddr  bd_addr;
    u16  ps_base;
    u16  size_psdata;
} CL_INTERNAL_SM_GET_ATTRIBUTE_REQ_T;

typedef struct
{
    u16  index;
    u16  ps_base;
    u16  size_psdata;
} CL_INTERNAL_SM_GET_INDEXED_ATTRIBUTE_REQ_T;

#ifndef DISABLE_BLE
typedef struct 
{
    Task                theAppTask;
    typed_bdaddr        taddr;
    ble_security_type   security;
    ble_connection_type conn_type;
}
CL_INTERNAL_SM_DM_SECURITY_REQ_T;
#endif 

/* State definition for the Connection Library */
typedef enum
{
    connectionUninitialised,
    connectionInitialising,
    connectionReady,
    connectionTestMode
} connectionStates;


/* Inquiry management state */
typedef struct
{
    Task nameLock;
#ifndef CL_EXCLUDE_INQUIRY
    Task inquiryLock;
    Task iacLock;
    unsigned periodic_inquiry:1;
#endif
} connectionInquiryState;


typedef struct
{
    Task                stateInfoLock;
    Sink                sink;
    
    /* Used to hold the bluetooth version of our device */
    cl_dm_bt_version    version;
} connectionReadInfoState;

typedef enum
{
    sm_init_set_none = 0,           /*!< None type */
    sm_init_set_security_mode,      /*!< Mode options set. */
    sm_init_set_security_config,    /*!< Config options set. */
    sm_init_set_security_default    /*!< Default security level options set. */
} sm_init_msg_type;

typedef enum
{
    /* ConnectionSmAddAuthrisedDevice() */
    device_req_add_authorised   = 0,
    /* ConnectionSmSetTrustedDevice() */
    device_req_set_trusted
} sm_device_req_type;

typedef struct
{
    /* Valid during the connectionInitialising state */
    unsigned            noDevices:4;
    unsigned            deviceCount:4;
    dm_security_mode    security_mode:8;
    encryption_mode     enc_mode:6;

    /* Used to indicate if security is through SM (TRUE) or LM (FALSE). */
    unsigned            sm_security:1;
    /* Used to indicate if this device is the responder (TRUE) or initiator 
     * (FALSE). 
     */
    unsigned            responder:1;
    /* Used to tell us what type of pairing we are performing */
    unsigned            authentication_requirements:4;

    sm_init_msg_type    sm_init_msg:4;

    u16              key_distribution;
    
    sm_device_req_type  deviceReqType:1;
    
    u16              TdlNumberOfDevices;
    
    /* Message primitives locks */
    Task                setSecurityModeLock;
    Task                authReqLock;
    Task                encryptReqLock;

    Task                deviceReqLock;

    /* Used to hold address of device we are Authenticating against */
    bdaddr              authRemoteAddr;
    
    /* Sink identifying the connecting being encrypted */
    Sink                sink;

    /* Cached permanent address returned by CL_DM_BLE_SECURITY_CFM when
     * bonding with a Privacy enabled BLE device using Resolvable Random
     * addressing.
     */
    typed_bdaddr        *permanent_taddr;
} connectionSmState;


typedef struct
{
    Task        sdpLock;
    Task        sdpSearchLock;
    bdaddr      sdpServerAddr;
} connectionSdpState;

typedef struct
{
    Sink roleLock;
} connectionLinkPolicyState;


typedef struct
{
    Task        mapLock;
} connectionL2capState;

typedef struct
{
    Task        txPwrLock;
} connectionReadTxPwrState;


/* Structure to hold the instance state for the Connection Library */
typedef struct
{
    TaskData                        task;     
    Task                            theAppTask; 
    connectionStates                state:8; /* uses only 2 bits */
    unsigned                        flags:8; /* see CONNECTION_FLAG_* */
    const msg_filter                *msgFilter;

    connectionInquiryState          inqState;
    connectionSmState               smState;
    connectionReadInfoState         infoState;
    connectionSdpState              sdpState;
    connectionLinkPolicyState       linkPolicyState;
    connectionL2capState            l2capState;
    connectionReadTxPwrState        readTxPwrState;
} connectionState;


/****************************************************************************
NAME    
    connectionGetCmTask

DESCRIPTION
    This function returns the connection library task so that the connection
    library can post a message to itself.

RETURNS
    The connection library task.
*/
Task connectionGetCmTask(void);

/****************************************************************************
NAME    
    connectionGetAppTask

DESCRIPTION
    This function returns the application task.

RETURNS
    The application task.
*/
Task connectionGetAppTask(void);

/****************************************************************************
NAME    
    connectionGetBtVersion

DESCRIPTION
    Returns the BT Version read from BlueStack during initialisation.

RETURNS
    cl_dm_bt_version
*/

cl_dm_bt_version connectionGetBtVersion(void);

/****************************************************************************
NAME    
    connectionGetMsgFilter

DESCRIPTION
    This function returns the connection library message filter.
*/
const msg_filter *connectionGetMsgFilter(void);


#endif  /* CONNECTION_PRIVATE_H_ */
