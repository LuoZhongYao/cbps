/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */
#ifndef CONNECTION_SHIM_LAYER_H
#define CONNECTION_SHIM_LAYER_H

#include <connection.h>


typedef struct
{
    Sink    sink;
    
    u16  size_data;
    u8   data[1];
} CL_SYSTEM_MORE_DATA_TEST_EXTRA_T;


typedef struct
{
    hci_status          status; 
    u16              transport;
    oob_data_setting    oob_data_present;
    u8               size_oob_data;
    /* Pointer to the start of the OOB data 
     * (hash c then rand r concatenated).
     */
    u8               oob_data[1];
} CL_SM_READ_LOCAL_OOB_DATA_CFM_TEST_EXTRA_T;

typedef struct
{
    Task     task;
    bdaddr   bd_addr;            
    u16      psm;                
    u16      cid;
    u16      identifier;     
} CL_L2CAP_CONNECT_IND_TEST_EXTRA_T;


typedef struct
{
    l2cap_connect_status status;
    u16                  psm_local;
    Sink                 sink;
    u16                  connection_id;
    u16                  mtu_remote;
    u16                  flush_timeout_remote;
    qos_flow             qos_remote;
    u8                   flow_mode;
    Task                 task;
} CL_L2CAP_CONNECT_CFM_TEST_EXTRA_T;

typedef struct
{
    u8           type;
    bdaddr          bd_addr;        
    bool            incoming;        
    u32          dev_class;
    hci_status      status;
    u16          flags;
    u16          conn_interval;
    u16          conn_latency;
    u16          supervision_timeout;
    u8           clock_accuracy;
} CL_DM_ACL_OPENED_IND_TEST_EXTRA_T;

typedef struct 
{
    u8           type;
    bdaddr          bd_addr;
    hci_status      status;
} CL_DM_ACL_CLOSED_IND_TEST_EXTRA_T;

typedef struct
{
    u8           type;
    bdaddr          bd_addr;
    u16          transport;
} CL_DM_APT_IND_TEST_EXTRA_T;

typedef struct
{
    u8           type;
    bdaddr          bd_addr;
    u16          transport;
} CL_SM_USER_PASSKEY_REQ_IND_TEST_EXTRA_T;

typedef struct
{
    u8           type;
    bdaddr          bd_addr;
    u16          transport;
    u32          passkey;
} CL_SM_USER_PASSKEY_NOTIFICATION_IND_TEST_EXTRA_T;


typedef struct
{
    u8           type;
    bdaddr          bd_addr;
    u16          transport;
    u32          numeric_value;
    bool            response_required;
} CL_SM_USER_CONFIRMATION_REQ_IND_TEST_EXTRA_T;

typedef struct
{
    u8           type;
    bdaddr          bd_addr;
} CL_SM_PIN_CODE_IND_TEST_EXTRA_T;

typedef struct
{
    hci_status      status;        
    Sink            sink;        
    u8           type;
    bdaddr          bd_addr;
    u16          transport;
} CL_SM_ENCRYPTION_KEY_REFRESH_IND_TEST_EXTRA_T;

typedef struct
{
    Sink            sink;
    bool            encrypted;
    u8           type;
    bdaddr          bd_addr;
    u16          transport;
} CL_SM_ENCRYPTION_CHANGE_IND_TEST_EXTRA_T;

typedef struct
{
    connection_lib_status   status;        
    u8                   type;
    bdaddr                  bd_addr;
    u16                  size_psdata;                
    u8                   psdata[1];                    
} CL_SM_GET_INDEXED_ATTRIBUTE_CFM_TEST_EXTRA_T;

typedef struct
{
    bool                    result;                
} CL_AUTH_SET_PRIORITY_DEVICE_IND_TEST_EXTRA_T;

typedef struct
{
    bool                    result;
    bool                    is_priority_device;
} CL_AUTH_GET_PRIORITY_DEVICE_STATUS_IND_TEST_EXTRA_T;

typedef struct
{
    bool                    result;
} CL_AUTH_IS_PRIORITY_DEVICE_IND_TEST_EXTRA_T;

typedef struct
{
    hci_status              status;
    u8                   rssi;
    u8                   type;
    bdaddr                  bd_addr;
    u16                  transport;
} CL_DM_RSSI_BDADDR_CFM_TEST_EXTRA_T;

typedef struct 
{
    u8                   type;
    bdaddr                  bd_addr;
    u16                  transport;
} CL_SM_IO_CAPABILITY_REQ_IND_TEST_EXTRA_T;

typedef struct
{
    u16                  authentication_requirements;
    u16                  io_capability;
    bool                    oob_data_present;
    u8                   type;
    bdaddr                  bd_addr;
    u16                  transport;
} CL_SM_REMOTE_IO_CAPABILITY_IND_TEST_EXTRA_T;

#ifndef DISABLE_BLE
typedef struct
{
    u8                           num_reports;         
    ble_advertising_event_type      event_type;
    u8                           current_addr_type;
    bdaddr                          current_addr;
    u8                           permanent_addr_type;
    bdaddr                          permanent_addr;
    i8                            rssi;
    u8                           size_ad_data;
    u8                           ad_data[1];
}
CL_DM_BLE_ADVERTISING_REPORT_IND_TEST_EXTRA_T;


typedef struct
{
    bool result;
} CL_BLE_ADD_ADVERTISING_FILTER_CFM_TEST_EXTRA_T;

typedef struct
{
    bool result;
} CL_BLE_CLEAR_ADVERTISING_FILTER_CFM_TEST_EXTRA_T;

typedef struct
{
    connection_lib_status   status;
    u8                   type;
    bdaddr                  bd_addr;
    u16                  transport;
    u16                  flags;
} CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_TEST_EXTRA_T;

typedef struct
{
    connection_lib_status   status;  
    u8                   type;
    bdaddr                  bd_addr;
} CL_DM_BLE_SECURITY_CFM_TEST_EXTRA_T;

typedef struct
{
    connection_lib_status       status;
    ble_local_addr_type         addr_type;
    u8                       type;
    bdaddr                      bd_addr;
} CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM_TEST_EXTRA_T;
#endif

#define CL_SHIM_MESSAGE_BASE
typedef enum
{
    CL_SYSTEM_MORE_DATA_TEST_EXTRA = CL_MESSAGE_TOP,
    CL_L2CAP_CONNECT_IND_TEST_EXTRA,
    CL_L2CAP_CONNECT_CFM_TEST_EXTRA,
    CL_SM_READ_LOCAL_OOB_DATA_CFM_TEST_EXTRA,
    CL_DM_ACL_OPENED_IND_TEST_EXTRA,
    CL_DM_ACL_CLOSED_IND_TEST_EXTRA,
    CL_DM_APT_IND_TEST_EXTRA,
    CL_SM_USER_PASSKEY_REQ_IND_TEST_EXTRA,
    CL_SM_USER_PASSKEY_NOTIFICATION_IND_TEST_EXTRA,
    CL_SM_USER_CONFIRMATION_REQ_IND_TEST_EXTRA,
    CL_SM_PIN_CODE_IND_TEST_EXTRA,
    CL_SM_ENCRYPTION_KEY_REFRESH_IND_TEST_EXTRA,
    CL_SM_ENCRYPTION_CHANGE_IND_TEST_EXTRA,
    CL_SM_GET_INDEXED_ATTRIBUTE_CFM_TEST_EXTRA,
    CL_AUTH_SET_PRIORITY_DEVICE_IND_TEST_EXTRA, 
    CL_AUTH_GET_PRIORITY_DEVICE_STATUS_IND_TEST_EXTRA, 
    CL_AUTH_IS_PRIORITY_DEVICE_IND_TEST_EXTRA, 
    CL_DM_RSSI_BDADDR_CFM_TEST_EXTRA,
    CL_SM_IO_CAPABILITY_REQ_IND_TEST_EXTRA,
    CL_SM_REMOTE_IO_CAPABILITY_IND_TEST_EXTRA,

#ifndef DISABLE_BLE
    CL_DM_BLE_ADVERTISING_REPORT_IND_TEST_EXTRA,
    CL_BLE_ADD_ADVERTISING_FILTER_CFM_TEST_EXTRA,
    CL_BLE_CLEAR_ADVERTISING_FILTER_CFM_TEST_EXTRA,
    CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND_TEST_EXTRA,
    CL_DM_BLE_SECURITY_CFM_TEST_EXTRA,
    CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM_TEST_EXTRA,
#endif

    CL_SHIM_MESSAGE_TOP
} ConnectionShimMessageId;

void ConnectionHandleComplexMessage(Task task, MessageId id, Message message);

void ConnectionL2capConnectRequestTestExtraDefault(
        Task theAppTask,
        const bdaddr *addr,
        u16 psm_local,
        u16 psm_remote
        );

void ConnectionL2capConnectRequestTestExtraConftab(
        Task theAppTask,
        const bdaddr *addr,
        u16 psm_local,
        u16 psm_remote,
        u16 size_conftab,
        u8 *conftab
        );

void ConnectionL2capConnectResponseTestExtraDefault(
        Task theAppTask,
        Task task,
        bool response,
        u16 psm,
        u16 cid,
        u8 identifier
        );

void ConnectionL2capConnectResponseTestExtraConftab(
        Task theAppTask,
        Task task,
        bool response,
        u16 psm,
        u16 cid,
        u8 identifier,
        u16 size_conftab,
        u8 *conftab
        );

void ConnectionRfcommConnectRequestTestExtraDefault(
        Task theAppTask,
        const bdaddr* bd_addr,
        u16 security_chan,
        u8 remote_server_chan
        );

void ConnectionRfcommConnectRequestTestExtraParams(
        Task theAppTask,
        const bdaddr* bd_addr,
        u16 security_chan,
        u8 remote_server_chan,
        u16 max_payload_size,
        u8 modem_signal,
        u8 break_signal,
        u16 msc_timeout
        );

void ConnectionRfcommConnectResponseTestExtraDefault(
        Task theAppTask,
        bool response,
        Sink sink,
        u8 local_server_channel
        );

void ConnectionRfcommConnectResponseTestExtraParams(
        Task theAppTask,
        bool response,
        Sink sink,
        u8 local_server_channel,
        u16 max_payload_size,
        u8 modem_signal,
        u8 break_signal,
        u16 msc_timeout
        );

void ConnectionRfcommPortNegRequestTestDefault(
        Task theAppTask,
        Sink sink,
        bool request
        );

void ConnectionRfcommPortNegResponseTestDefault(Task theAppTask, Sink sink);

void ConnectionRfcommPortNegRequestTestExtra(
        Task theAppTask,
        Sink sink,
        bool request,
        u8 baud_rate,
        u8 data_bits,
        u8 stop_bits,
        u8 parity,
        u8 parity_type,
        u8 flow_ctrl_mask,
        u8 xon,
        u8 xoff,
        u16 parameter_mask
        );

void ConnectionRfcommPortNegResponseTestExtra(
        Task theAppTask,
        Sink sink,
        u8 baud_rate,
        u8 data_bits,
        u8 stop_bits,
        u8 parity,
        u8 parity_type,
        u8 flow_ctrl_mask,
        u8 xon,
        u8 xoff,
        u16 parameter_mask
        );


void ConnectionSyncConnectRequestTestExtraDefault(Task theAppTask, Sink sink);

void ConnectionSyncConnectRequestTestExtraParams(
        Task theAppTask,
        Sink sink,
        u32 tx_bandwidth,
        u32 rx_bandwidth,
        u16 max_latency,
        u16 voice_settings,
        sync_retx_effort retx_effort,
        sync_pkt_type packet_type
        );

void ConnectionSyncConnectResponseTestExtraDefault(
        Task theAppTask,
        const bdaddr* bd_addr,
        bool accept
        );

void ConnectionSyncConnectResponseTestExtraParams(
        Task theAppTask,
        const bdaddr* bd_addr,
        bool accept,
        u32 tx_bandwidth,
        u32 rx_bandwidth,
        u16 max_latency,
        u16 voice_settings,
        sync_retx_effort retx_effort,
        sync_pkt_type packet_type
        );

void ConnectionSyncRenegotiateTestExtraDefault(Task theAppTask, Sink sink);

void ConnectionSyncRenegotiateTestExtraParams(
        Task theAppTask,
        Sink sink,
        u32 tx_bandwidth,
        u32 rx_bandwidth,
        u16 max_latency,
        u16 voice_settings,
        sync_retx_effort retx_effort,
        sync_pkt_type packet_type
        );

void ConnectionSdpServiceSearchAttributeRequestTestExtra(
        Task theAppTask,
        const bdaddr *addr,
        u16 max_attributes,
        u16 size_search_pattern,
        u16 size_attribute_list,
        u16 size_search_attribute_list,
        const u8 *search_attribute_list
        );

void ConnectionSendSinkDataTestExtra(Sink sink, u16 size_data, u8* data);

void ConnectionWriteInquiryAccessCodeTestExtra(
        Task theAppTask,
        u8 *iac,
        u16 num_iac
        );

void ConnectionSmIoCapabilityResponseTestExtra(
        u8               type,
        bdaddr*             bd_addr,
        u16              transport,
        cl_sm_io_capability io_capability,
        u16              mitm,
        bool                bonding,
        u16              key_distribution,
        u16              oob_setting,
        u8               size_oob_data,
        u8*              oob_data
        );

void ConnectionSmIoCapabilityResponseTestExtraDefault(
        bdaddr* bd_addr,
        cl_sm_io_capability io_capability,
        u16 mitm,
        bool bonding
        );

void ConnectionSendSinkAutoDataTestExtra(Sink sink, u16 size_data);

void ConnectionEnterDutModeTestExtra(u8 dummy);

void ConnectionReadRemoteVersionBdaddrTestExtra(
        Task theAppTask,
        u8 bdaddr_typed,
        const bdaddr * addr
        );

void ConnectionSmAddAuthDeviceTestExtra(
        Task theAppTask,
        const bdaddr *peer_bd_addr,
        u16 trusted,
        u16 bonded,
        u8 key_type,
        u16 size_link_key,
        const u8* link_key
        );

#ifndef DISABLE_BLE

void ConnectionBleAddAdvertisingReportFilterTestExtra(
        Task theAppTask,
        ble_ad_type ad_type,
        u16 interval,
        u16 size_pattern,
        const u8 * pattern
        );

void ConnectionBleClearAdvertisingReportFilterTestExtra(
        Task theAppTask,
        u16 dummy
        );

void ConnectionDmBleSecurityReqTestExtra(
        Task                    theAppTask, 
        u8                   type, 
        const bdaddr            *addr, 
        ble_security_type       security,
        ble_connection_type     conn_type
        );

void ConnectionSetLinkPolicyTestExtra(u8 dummy);

void ConnectionDmBleSetAdvertisingParamsReqTestExtraDefault(
        ble_adv_type            adv_type,
        bool                    random_own_address,
        u8                   channel_map,
        u16                  adv_interval_min,
        u16                  adv_interval_max, 
        ble_adv_filter_policy   filter_policy
        );

void ConnectionDmBleSetAdvertisingParamsReqTestExtra(
        bool                    random_own_address,
        u8                   channel_map,
        bool                    random_direct_address,
        const bdaddr            *bd_addr
        );

void ConnectionL2capConnectionParametersUpdateReqTestExtra(
        u8                   type,
        const bdaddr            *addr,
        u16                  min_interval,
        u16                  max_interval,
        u16                  latency,
        u16                  timeout
        );

void ConnectionDmBleSetConnectionParametersReqTestExtra(
        u16      size_param_arr,
        const u8 *param_arr
        );

void ConnectionDmBleAddDeviceToWhiteListReqTestExtra(
        u8 type, 
        const bdaddr *bd_addr
        );

void ConnectionDmBleRemoveDeviceFromWhiteListReqTestExtra(
        u8 type,
        const bdaddr *bd_addr
        );

void ConnectionDmBleReadWhiteListSizeReqTestExtra(u8 dummy);

void ConnectionDmBleClearWhiteListReqTestExtra(u8 dummy);

void ConnectionDmBleConfigureLocalAddressReqTestExtra(
        u16                  local,
        u8                   type,
        const bdaddr            *bd_addr    
        );

void ConnectionDmBleConnectionParametersUpdateReqTestExtra(
        Task theAppTask,
        u8  type,
        bdaddr *bd_addr,
        u16 min_interval,
        u16 max_interval,
        u16 latency,
        u16 timeout,
        u16 min_ce_length,
        u16 max_ce_length
        );        

void ConnectionDmBleAcceptConnectionParUpdateResponseTestExtra(
        bool                accept_update,
        u8               type,
        const bdaddr        *bd_addr,
        u16              id,
        u16              conn_interval_min,
        u16              conn_interval_max,
        u16              conn_latency,
        u16              supervision_timeout
        );

#endif /* DISABLE_BLE */

void ConnectionGetRssiBdaddrTestExtraDefault(
        Task            theAppTask,
        u8           type,
        const bdaddr    *bd_addr,
        TRANSPORT_T     transport
        );

void ConnectionSmUserConfirmationResponseTestExtra(
        u8           type,
        const bdaddr*   bd_addr,
        u16          transport,
        bool            confirm
        );

void ConnectionSmUserPasskeyResponseTestExtra(
        u8           type,
        const bdaddr*   bd_addr,
        u16          transport,
        bool            cancelled,
        u32          passkey
        );

void ConnectionSmSendKeypressNotificationRequestTestExtra(
        u8               type,
        const bdaddr*       bd_addr,
        u16              transport,
        u16              keypress_type
        );

void ConnectionSmEncryptionKeyRefreshTestExtra(
        u8 type, 
        const bdaddr* bd_addr
        );

void ConnectionSmPinCodeResponseTestExtra(
        u8           type,
        const bdaddr*   bd_addr,
        u16          size_pin_code,
        const u8*    pin_code
        );
        
/* 
 *  B-134381
 */
void ConnectionAuthSetPriorityDeviceTestExtra(
        Task theAppTask,
        const bdaddr *bd_addr,
        bool is_priority_device
        );

/* 
 *  B-185975
 */

void ConnectionAuthGetPriorityDeviceStatusTestExtra(
        Task theAppTask,
        const bdaddr* bd_addr, 
        bool *is_priority_device
        );

void ConnectionAuthIsPriorityDeviceTestExtra(
        Task theAppTask,
        const bdaddr* bd_addr
        );        
        
#endif /* CONNECTION_SHIM_LAYER_H */
