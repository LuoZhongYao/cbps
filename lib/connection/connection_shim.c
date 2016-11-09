/* Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */
#include <panic.h>
#include <message.h>
#include <stdlib.h>
#include <string.h> /* for memset */
#include <stream.h>
#include <source.h>
#include <sink.h>
#include <vm.h>
#include "connection_shim.h"

#include <stdio.h>

/* Macro for flattening tp_bdaddr and typed_bdaddr. Assumes that 
 *     typed_bdaddr taddr; 
 * has the same shape and position as 
 *     u8        type; 
 *     bdaddr       bd_addr;
 * and that
 *     tp_bdaddr    tpaddr;
 * has the same shape and position as
 *     u8        type;
 *     bdaddr       bd_addr;
 *     TRANSPORT_T  transport
 * in the _TEST_EXTRA_ version of the msg structure.
 */
#define FLATTEN_BDADDR(MSG) \
    case MSG:\
    {\
        MSG##_T *original =  (MSG##_T *)message;\
        MSG##_TEST_EXTRA_T *new_msg = (MSG##_TEST_EXTRA_T *) \
            PanicUnlessMalloc(sizeof(MSG##_TEST_EXTRA_T));\
        memmove( new_msg, original, sizeof(MSG##_TEST_EXTRA_T));\
        MessageSend(task, MSG##_TEST_EXTRA, new_msg);\
    }\
    break

void ConnectionHandleComplexMessage(Task task, MessageId id, Message message)
{
    switch (id)
    {
        case CL_L2CAP_CONNECT_IND:
            {
                CL_L2CAP_CONNECT_IND_T *original;
                CL_L2CAP_CONNECT_IND_TEST_EXTRA_T *new_msg = 
                    malloc(sizeof(CL_L2CAP_CONNECT_IND_TEST_EXTRA_T));
                original = (CL_L2CAP_CONNECT_IND_T *) message;
            
                new_msg->bd_addr = original->bd_addr;
                new_msg->psm = original->psm;
                new_msg->cid = original->connection_id;
                new_msg->identifier = original->identifier;
                new_msg->task = task;
                MessageSend(task, CL_L2CAP_CONNECT_IND_TEST_EXTRA, new_msg);
            }
            break;

        case CL_L2CAP_CONNECT_CFM:
            {
                CL_L2CAP_CONNECT_CFM_T *original;
                CL_L2CAP_CONNECT_CFM_TEST_EXTRA_T *new_msg = 
                    malloc(sizeof(CL_L2CAP_CONNECT_CFM_TEST_EXTRA_T));
                original = (CL_L2CAP_CONNECT_CFM_T *) message;

                new_msg->status = original->status;
                new_msg->psm_local = original->psm_local;
                new_msg->sink = original->sink;
                new_msg->connection_id = original->connection_id;
                new_msg->mtu_remote = original->mtu_remote;
                new_msg->flush_timeout_remote = original->flush_timeout_remote;
                new_msg->qos_remote = original->qos_remote;
                new_msg->flow_mode = original->mode;
                new_msg->task = MessageSinkGetTask(original->sink);
                MessageSend(task, CL_L2CAP_CONNECT_CFM_TEST_EXTRA, new_msg);
            }
            break;
            
        case MESSAGE_MORE_DATA:
            {
                Source src = ((MessageMoreData*)message)->source;
                CL_SYSTEM_MORE_DATA_TEST_EXTRA_T *pdu;
                const u8 *s = SourceMap(src);
                u16 len = SourceBoundary(src);
                u16 datalen=len;
                
                /* Do not allow large data more than 512 ..Just Limit to 
                 * 128 bytes.
                 */
                if(len > 128) len=128;
                
                pdu = malloc(sizeof(CL_SYSTEM_MORE_DATA_TEST_EXTRA_T)+len);
                memset(pdu, 0, sizeof(CL_SYSTEM_MORE_DATA_TEST_EXTRA_T));
                
                pdu->sink = StreamSinkFromSource(src);
                pdu->size_data = datalen;
                
                memmove(pdu->data, s, len);
                
                SourceDrop(src, datalen);
                
                MessageSend(task, CL_SYSTEM_MORE_DATA_TEST_EXTRA, pdu);
            }
                break;
            
        case MESSAGE_MORE_SPACE:
        case MESSAGE_SOURCE_EMPTY:
            break;

        case CL_SM_READ_LOCAL_OOB_DATA_CFM:
            {
                /* Create new message */
                CL_SM_READ_LOCAL_OOB_DATA_CFM_T *original;
                CL_SM_READ_LOCAL_OOB_DATA_CFM_TEST_EXTRA_T *new_msg = 
                    malloc(
                        sizeof(CL_SM_READ_LOCAL_OOB_DATA_CFM_TEST_EXTRA_T) 
                        + (2 * CL_SIZE_OOB_DATA)
                        );
                original = (CL_SM_READ_LOCAL_OOB_DATA_CFM_T*)message;
                /* Copy over the status */
                new_msg->status = original->status;
                /* Copy over transport */
                new_msg->transport = original->transport;
                /* Copy over what type of OOB data is present */
                new_msg->oob_data_present = original->oob_data;
                /* Set size */
                new_msg->size_oob_data = 2*CL_SIZE_OOB_DATA;
                /* Copy over the OOB data */
                memmove(
                    new_msg->oob_data,
                    original->oob_hash_c,
                    CL_SIZE_OOB_DATA
                    );
                memmove(
                    new_msg->oob_data+CL_SIZE_OOB_DATA,
                    original->oob_rand_r,
                    CL_SIZE_OOB_DATA
                    );
                /* Send it to the app */
                MessageSend(
                    task,
                    CL_SM_READ_LOCAL_OOB_DATA_CFM_TEST_EXTRA,
                    new_msg
                    );
            }
                break;

        /* Flatten CL_SM_GET_INDEXED_ATTRIBUTE_CFM */
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
        {
            CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T *original = 
                (CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T *) message;
            CL_SM_GET_INDEXED_ATTRIBUTE_CFM_TEST_EXTRA_T *new_msg = 
                malloc(
                    sizeof(CL_SM_GET_INDEXED_ATTRIBUTE_CFM_TEST_EXTRA_T) 
                    + (original->size_psdata * sizeof(u8))
                    - 1     /* for the psdata array of 1 */
                    );

            new_msg->status = original->status;
            new_msg->type = original->taddr.type;
            new_msg->bd_addr = original->taddr.addr;
            new_msg->size_psdata = original->size_psdata;
            memmove(new_msg->psdata, original->psdata, original->size_psdata);

            /* Send the copied message to the app. */
            MessageSend(
                task,
                CL_SM_GET_INDEXED_ATTRIBUTE_CFM_TEST_EXTRA,
                new_msg
                );
        }
            break;
                    
        FLATTEN_BDADDR(CL_DM_ACL_OPENED_IND); 
        FLATTEN_BDADDR(CL_DM_ACL_CLOSED_IND);
        FLATTEN_BDADDR(CL_DM_APT_IND);
        FLATTEN_BDADDR(CL_SM_USER_PASSKEY_REQ_IND);
        FLATTEN_BDADDR(CL_SM_USER_PASSKEY_NOTIFICATION_IND);
        FLATTEN_BDADDR(CL_SM_USER_CONFIRMATION_REQ_IND);
        FLATTEN_BDADDR(CL_SM_PIN_CODE_IND);
        FLATTEN_BDADDR(CL_SM_ENCRYPTION_KEY_REFRESH_IND);
        FLATTEN_BDADDR(CL_SM_ENCRYPTION_CHANGE_IND);
        FLATTEN_BDADDR(CL_SM_IO_CAPABILITY_REQ_IND);
        FLATTEN_BDADDR(CL_SM_REMOTE_IO_CAPABILITY_IND);

#ifndef DISABLE_BLE
        /* Flatten the typed_bdaddr parts of the message - grrr! */
        case CL_DM_BLE_ADVERTISING_REPORT_IND:
        {
            CL_DM_BLE_ADVERTISING_REPORT_IND_T *original = 
                (CL_DM_BLE_ADVERTISING_REPORT_IND_T *)message;
            CL_DM_BLE_ADVERTISING_REPORT_IND_TEST_EXTRA_T *new_msg = 
                (CL_DM_BLE_ADVERTISING_REPORT_IND_TEST_EXTRA_T *) 
                    PanicUnlessMalloc(
                        sizeof(CL_DM_BLE_ADVERTISING_REPORT_IND_TEST_EXTRA_T) +
                        original->size_advertising_data - 1
                        );
            new_msg->num_reports = original->num_reports;
            new_msg->event_type  = original->event_type;
            new_msg->current_addr_type = original->current_taddr.type;
            new_msg->current_addr = original->current_taddr.addr;
            new_msg->permanent_addr_type = original->permanent_taddr.type;
            new_msg->permanent_addr = original->permanent_taddr.addr;
            new_msg->rssi = original->rssi;

            new_msg->size_ad_data = original->size_advertising_data;
            memmove(
                new_msg->ad_data,
                original->advertising_data,
                original->size_advertising_data
                );
            MessageSend(
                task, 
                CL_DM_BLE_ADVERTISING_REPORT_IND_TEST_EXTRA, 
                new_msg
                );
        }
        break;

        FLATTEN_BDADDR(CL_SM_BLE_SIMPLE_PAIRING_COMPLETE_IND);
        FLATTEN_BDADDR(CL_DM_BLE_SECURITY_CFM);
        FLATTEN_BDADDR(CL_DM_BLE_CONFIGURE_LOCAL_ADDRESS_CFM);
#endif

        default:
            printf("CL_MESSAGE_TOP 0x%X\n", CL_MESSAGE_TOP);
            printf("CL_SHIM_MESSAGE_TOP 0x%X\n", CL_SHIM_MESSAGE_TOP);
            printf("Message received id 0x%X\n", id);
            Panic();
            break;
    }
}

void ConnectionL2capConnectRequestTestExtraDefault(
        Task theAppTask,
        const bdaddr *addr,
        u16 psm_local,
        u16 psm_remote
        )
{
    ConnectionL2capConnectRequest(
            theAppTask,
            addr,
            psm_local,
            psm_remote,
            0,
            0
            );
}

void ConnectionL2capConnectRequestTestExtraConftab(
        Task theAppTask,
        const bdaddr *addr,
        u16 psm_local,
        u16 psm_remote,
        u16 size_conftab,
        u8 *conftab
        )
{
    if (size_conftab)
    {
        /* copy the conftab data to a slot */
        u16* dyn_conftab = malloc(sizeof(u16)* (size_conftab/2));
        u16 it;

        for(it=0; it<size_conftab/2; it++)
        {
            dyn_conftab[it] =  *conftab++ << 8;
            dyn_conftab[it] |= *conftab++;
        }

        ConnectionL2capConnectRequest(
            theAppTask, addr, psm_local, psm_remote,
            (size_conftab/2), dyn_conftab);
    }
    else
    {
        ConnectionL2capConnectRequest(
            theAppTask, addr, psm_local, psm_remote,
            0, 0);
    }        
}

void ConnectionL2capConnectResponseTestExtraDefault(
        Task theAppTask,
        Task task,
        bool response,
        u16 psm,
        u16 cid,
        u8 identifier
        )
{
    ConnectionL2capConnectResponse(
            task,
            response,
            psm,
            cid,
            identifier,
            0,
            0
            );
}

void ConnectionL2capConnectResponseTestExtraConftab(
        Task theAppTask,
        Task task,
        bool response,
        u16 psm,
        u16 cid,
        u8 identifier,
        u16 size_conftab,
        u8 *conftab
        )
{
    if (size_conftab)
    {
        /* copy the conftab data to a slot */
        u16* dyn_conftab = malloc(sizeof(u16)* (size_conftab/2));
        u16 it;

        for(it=0; it<size_conftab/2; it++)
        {
            dyn_conftab[it] =  *conftab++ << 8;
            dyn_conftab[it] |= *conftab++;
        }

        ConnectionL2capConnectResponse(
            (Task)task, response, psm, cid, identifier, 
            (size_conftab/2), dyn_conftab);

    }
    else
    {
        ConnectionL2capConnectResponse(
            (Task)task, response, psm, cid, identifier, 
             0, 0);
    }

}


void ConnectionRfcommConnectRequestTestExtraDefault(
        Task theAppTask,
        const bdaddr* bd_addr,
        u16 security_chan,
        u8 remote_server_chan
        )
{
    ConnectionRfcommConnectRequest(
            theAppTask,
            bd_addr,
            security_chan,
            remote_server_chan,
            0
            );
}

void ConnectionRfcommConnectRequestTestExtraParams(
        Task theAppTask,
        const bdaddr* bd_addr,
        u16 security_chan,
        u8 remote_server_chan,
        u16 max_payload_size,
        u8 modem_signal,
        u8 break_signal,
        u16 msc_timeout
        )
{
    rfcomm_config_params params;  

    params.max_payload_size = max_payload_size;
    params.modem_signal     = modem_signal;
    params.break_signal     = break_signal;
    params.msc_timeout      = msc_timeout ; 
    
    ConnectionRfcommConnectRequest(
            theAppTask,
            bd_addr,
            security_chan,
            remote_server_chan,
            &params
            );
}

void ConnectionRfcommConnectResponseTestExtraDefault(
        Task theAppTask,
        bool response,
        Sink sink,
        u8 local_server_channel
        )
{
    ConnectionRfcommConnectResponse(
            theAppTask,
            response,
            sink,
            local_server_channel,
            0
            );
}

void ConnectionRfcommConnectResponseTestExtraParams(
        Task theAppTask,
        bool response,
        Sink sink,
        u8 local_server_channel,
        u16 max_payload_size,
        u8 modem_signal,
        u8 break_signal,
        u16 msc_timeout
        )
{
    rfcomm_config_params params;  

    params.max_payload_size = max_payload_size;
    params.modem_signal     = modem_signal;
    params.break_signal     = break_signal;
    params.msc_timeout      = msc_timeout ; 

    ConnectionRfcommConnectResponse(
            theAppTask,
            response,
            sink,
            local_server_channel,
            &params
            );
}

void ConnectionRfcommPortNegRequestTestDefault(
        Task theAppTask,
        Sink sink, 
        bool request
        )
{
    ConnectionRfcommPortNegRequest(theAppTask, sink, request, 0);
}

void ConnectionRfcommPortNegResponseTestDefault(Task theAppTask, Sink sink)
{
    ConnectionRfcommPortNegResponse(theAppTask, sink, 0);
}


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
        )
{
    port_par p_params;

    p_params.baud_rate = baud_rate;
    p_params.data_bits = data_bits;
    p_params.stop_bits = stop_bits;
    p_params.parity = parity;
    p_params.parity_type = parity_type;
    p_params.flow_ctrl_mask = flow_ctrl_mask;
    p_params.xon = xon;
    p_params.xoff = xoff;
    p_params.parameter_mask = parameter_mask;

    ConnectionRfcommPortNegRequest(theAppTask, sink, request, &p_params);
}

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
        )
{
    port_par p_params;

    p_params.baud_rate = baud_rate;
    p_params.data_bits = data_bits;
    p_params.stop_bits = stop_bits;
    p_params.parity = parity;
    p_params.parity_type = parity_type;
    p_params.flow_ctrl_mask = flow_ctrl_mask;
    p_params.xon = xon;
    p_params.xoff = xoff;
    p_params.parameter_mask = parameter_mask;

    ConnectionRfcommPortNegResponse(theAppTask, sink, &p_params);
}

void ConnectionSyncConnectRequestTestExtraDefault(Task theAppTask, Sink sink)
{
    ConnectionSyncConnectRequest(theAppTask, sink, NULL);
}


void ConnectionSyncConnectRequestTestExtraParams(
        Task theAppTask,
        Sink sink,
        u32 tx_bandwidth,
        u32 rx_bandwidth,
        u16 max_latency,
        u16 voice_settings,
        sync_retx_effort retx_effort,
        sync_pkt_type packet_type
        )
{
    sync_config_params config;

    config.tx_bandwidth = tx_bandwidth;
    config.rx_bandwidth = rx_bandwidth;
    config.max_latency = max_latency;
    config.voice_settings = voice_settings;
    config.retx_effort = retx_effort;
    config.packet_type = packet_type;

    ConnectionSyncConnectRequest(theAppTask, sink, &config);
}


void ConnectionSyncConnectResponseTestExtraDefault(
        Task theAppTask,
        const bdaddr* bd_addr,
        bool accept
        )
{
    ConnectionSyncConnectResponse(theAppTask, bd_addr, accept, NULL);
}


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
        )
{
    sync_config_params config;

    config.tx_bandwidth = tx_bandwidth;
    config.rx_bandwidth = rx_bandwidth;
    config.max_latency = max_latency;
    config.voice_settings = voice_settings;
    config.retx_effort = retx_effort;
    config.packet_type = packet_type;

    ConnectionSyncConnectResponse(theAppTask, bd_addr, accept, &config);
}


void ConnectionSyncRenegotiateTestExtraDefault(Task theAppTask, Sink sink)
{
    ConnectionSyncRenegotiate(theAppTask, sink, NULL);
}


void ConnectionSyncRenegotiateTestExtraParams(
        Task theAppTask,
        Sink sink,
        u32 tx_bandwidth,
        u32 rx_bandwidth,
        u16 max_latency,
        u16 voice_settings,
        sync_retx_effort retx_effort,
        sync_pkt_type packet_type
        )
{
    sync_config_params config;

    config.tx_bandwidth = tx_bandwidth;
    config.rx_bandwidth = rx_bandwidth;
    config.max_latency = max_latency;
    config.voice_settings = voice_settings;
    config.retx_effort = retx_effort;
    config.packet_type = packet_type;

    ConnectionSyncRenegotiate(theAppTask, sink, &config);
}


void ConnectionSdpServiceSearchAttributeRequestTestExtra(
        Task theAppTask,
        const bdaddr *addr,
        u16 max_attributes,
        u16 size_search_pattern,
        u16 size_attribute_list,
        u16 size_search_attribute_list,
        const u8 *search_attribute_list
        )
{
    /* Unused in the shim layer but needed to generate the message from rfcli */
    size_search_attribute_list = size_search_attribute_list;

    ConnectionSdpServiceSearchAttributeRequest(
            theAppTask,
            addr,
            max_attributes,
            size_search_pattern,
            search_attribute_list,
            size_attribute_list,
            search_attribute_list+size_search_pattern
            );
}

void ConnectionSendSinkDataTestExtra(Sink sink, u16 size_data, u8* data)
{
    u8* s=SinkMap(sink);
    u16 o=SinkClaim(sink,size_data);

    memmove(s+o, data, size_data);
    
    SinkFlush(sink, size_data);
}

void ConnectionSendSinkAutoDataTestExtra(Sink sink, u16 size_data)
{
    u8* s=SinkMap(sink);
    u16 o;

    printf("Data to send of size %d",size_data);
    o= SinkClaim(sink,size_data);
    if (0xFFFF == o) 
    {
        printf("Failed to Claim Sink for Data size %d",size_data);
        return;            
    }

    /* Memset with the same value */
    memset(s+o, 0x31, size_data);
    printf("Sending Data of size %d",size_data);
    
    SinkFlush(sink, size_data);
}

void ConnectionWriteInquiryAccessCodeTestExtra(
        Task theAppTask,
        u8 *iac,
        u16 num_iac
        )
{
    /* TODO: Implement shim function */
}

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
        )
{
    tp_bdaddr tpaddr;
    u16 rand_r_offset = 0;
    u8  *rand_r_ptr = NULL;

    tpaddr.transport = (TRANSPORT_T)transport;
    tpaddr.taddr.type = type;
    tpaddr.taddr.addr = *bd_addr;

    if (oob_setting & oob_data_p256)
        rand_r_offset += CL_SIZE_OOB_DATA;

    if ( 
        transport == TRANSPORT_BREDR_ACL && 
        (oob_setting & oob_data_p192)
       )
        rand_r_offset += CL_SIZE_OOB_DATA;

    if (rand_r_offset)
        rand_r_ptr = oob_data + rand_r_offset;
        
    ConnectionSmIoCapabilityResponse(
            &tpaddr,
            io_capability,
            mitm,
            bonding,
            key_distribution,
            oob_setting,
            oob_data,
            rand_r_ptr
            );
}


void ConnectionSmIoCapabilityResponseTestExtraDefault(
        bdaddr* bd_addr,
        cl_sm_io_capability io_capability,
        u16 mitm,
        bool bonding
        )
{
    tp_bdaddr tpaddr;

    tpaddr.taddr.type = TYPED_BDADDR_PUBLIC;
    tpaddr.taddr.addr = *bd_addr;
    tpaddr.transport = TRANSPORT_BREDR_ACL;

    ConnectionSmIoCapabilityResponse(
            &tpaddr,
            io_capability,
            mitm,
            bonding,
            0,              /* Key distribution */
            0,              /* OOB Data setting */
            0,              /* OOB HASH_C */
            0               /* OOB RAND_R */
            );
}

/* This is done purely to work around a bug in RFLCI
 * See B-80970 for the original bug.
 */
void ConnectionEnterDutModeTestExtra(u8 dummy)
{
    ConnectionEnterDutMode();
}

/* Make a shim but typed_bdaddr needs to be supported in the same way as bdaddr 
 * in the future.
 */
void ConnectionReadRemoteVersionBdaddrTestExtra(
        Task theAppTask,
        u8 bdaddr_typed,
        const bdaddr * addr
        )
{
    /* TODO: Implement shim Function */
}

/* RFCLI can't handle arrays of u16 so need to pass an array of u8 and 
 * Pack it.
 */
void ConnectionSmAddAuthDeviceTestExtra(
        Task theAppTask,
        const bdaddr *peer_bd_addr,
        u16 trusted,
        u16 bonded,
        u8 key_type,
        u16 size_link_key,
        const u8* link_key
        )
{
    u16 *u16_link_key = malloc( sizeof(u16) * size_link_key/2);
    u16 *ptr = u16_link_key;
    u16 idx;

    for(idx=0; idx<size_link_key; idx+=2)
    {
        *(ptr++) = ((u16)link_key[idx] << 8) | (link_key[idx+1]);
    }
    
    ConnectionSmAddAuthDevice(
        theAppTask,
        peer_bd_addr,
        trusted,
        bonded,
        key_type,
        size_link_key/2,
        u16_link_key);

    free(u16_link_key);
}

#ifndef DISABLE_BLE

void ConnectionBleAddAdvertisingReportFilterTestExtra(
        Task theAppTask,
        ble_ad_type ad_type,
        u16 interval,
        u16 size_pattern,
        const u8 * pattern
        )
{
    CL_BLE_ADD_ADVERTISING_FILTER_CFM_TEST_EXTRA_T * cfm = 
        PanicUnlessNew(CL_BLE_ADD_ADVERTISING_FILTER_CFM_TEST_EXTRA_T);

    cfm->result = ConnectionBleAddAdvertisingReportFilter(
                        ad_type,
                        interval,
                        size_pattern,
                        pattern
                        );
    
    MessageSend(theAppTask,CL_BLE_ADD_ADVERTISING_FILTER_CFM_TEST_EXTRA, cfm);
}

/* Another dummy param to stop RFCLI breaking */
void ConnectionBleClearAdvertisingReportFilterTestExtra(
        Task theAppTask,
        u16 dummy
        )
{
    CL_BLE_CLEAR_ADVERTISING_FILTER_CFM_TEST_EXTRA_T * cfm = 
        PanicUnlessNew(CL_BLE_CLEAR_ADVERTISING_FILTER_CFM_TEST_EXTRA_T);

    dummy = dummy;

    cfm->result = ConnectionBleClearAdvertisingReportFilter();
    
    MessageSend(theAppTask,CL_BLE_CLEAR_ADVERTISING_FILTER_CFM_TEST_EXTRA, cfm);
}

/* Flatten the typed_bdaddr type for BLE functions calls */
void ConnectionDmBleSecurityReqTestExtra(
        Task                    theAppTask, 
        u8                   type, 
        const bdaddr            *addr, 
        ble_security_type       security,
        ble_connection_type     conn_type
        )
{
    typed_bdaddr taddr;
    taddr.type = type;
    taddr.addr = *addr;
    ConnectionDmBleSecurityReq(
        theAppTask,
        &taddr,
        security,
        conn_type
        );
}

/* If this white list function is not wrapped, the BDADDR is junk - Don't
 * know why!
 */
void ConnectionDmBleAddDeviceToWhiteListReqTestExtra(
        u8 type,
        const bdaddr *bd_addr
        )
{
    ConnectionDmBleAddDeviceToWhiteListReq(type, bd_addr);
}

/* If this white list function is not wrapped, the BDADDR is junk - Don't
 * know why!
 */
void ConnectionDmBleRemoveDeviceFromWhiteListReqTestExtra(
        u8 type,
        const bdaddr *bd_addr
        )
{
    ConnectionDmBleRemoveDeviceFromWhiteListReq(type, bd_addr);
}

/* Dummy Param to stop RFCLI from breaking. */
void ConnectionDmBleReadWhiteListSizeReqTestExtra(u8 dummy)
{
    ConnectionDmBleReadWhiteListSizeReq();
}

/* Dummy Param to stop RFCLI from breaking. */
void ConnectionDmBleClearWhiteListReqTestExtra(u8 dummy)
{
    ConnectionDmBleClearWhiteListReq();
}

void ConnectionSetLinkPolicyTestExtra(u8 dummy)
{
    /* stub! */
}

/* Used for undirected advertising params */
void ConnectionDmBleSetAdvertisingParamsReqTestExtraDefault(
        ble_adv_type            adv_type,
        bool                    random_own_address,
        u8                   channel_map,
        u16                  adv_interval_min,
        u16                  adv_interval_max, 
        ble_adv_filter_policy   filter_policy
        )
{
    ble_adv_params_t *params = PanicUnlessNew(ble_adv_params_t);

    params->undirect_adv.adv_interval_min = adv_interval_min;
    params->undirect_adv.adv_interval_max = adv_interval_max;
    params->undirect_adv.filter_policy    = filter_policy;

    ConnectionDmBleSetAdvertisingParamsReq(
            adv_type,
            random_own_address,
            channel_map,
            params
            );
    free(params);
}

/* Use for directed advertising params */
void ConnectionDmBleSetAdvertisingParamsReqTestExtra(
        bool                    random_own_address,
        u8                   channel_map,
        bool                    random_direct_address,
        const bdaddr            *bd_addr
        )
{
    ble_adv_params_t *params = PanicUnlessNew(ble_adv_params_t);

    params->direct_adv.random_direct_address = random_direct_address;
    memmove(&params->direct_adv.direct_addr, bd_addr, sizeof(bdaddr));

    /* ble_adv_type is fixed to direct advertising.*/
    ConnectionDmBleSetAdvertisingParamsReq(
            ble_adv_direct_ind,
            random_own_address,
            channel_map,
            params
            );
    free(params);
}


void ConnectionDmBleSetConnectionParametersReqTestExtra(
        u16      size_param_arr,
        const u8 *param_arr
        )
{
    ble_connection_params *params = PanicUnlessNew(ble_connection_params);

    u16 size_arr =  size_param_arr/2;
    u16 *u16_array = malloc( sizeof(u16) * size_arr);
    u16 *ptr = u16_array;
    u16 idx;

    for(idx=0; idx<size_param_arr; idx+=2)
    {
        *(ptr++) = ((u16)param_arr[idx] << 8) | (param_arr[idx+1]);
    }

    if (size_arr > sizeof(ble_connection_params))
        size_arr = sizeof(ble_connection_params);

    /* setup defaults, in case the array falls short. */
    params->scan_interval           = 0x0010;
    params->scan_window             = 0x0010;
    params->conn_interval_min       = 0x0010;
    params->conn_interval_max       = 0x0010;
    params->conn_latency            = 0x0040;
    params->supervision_timeout     = 0x0BB8;   /* 30-seconds */
    params->conn_attempt_timeout    = 0x03E8;   /* 10-seconds */
    params->conn_latency_max        = 0x0040;
    params->supervision_timeout_min = 0x01F4;   /* 5 seconds */
    params->supervision_timeout_max = 0x0c80;   /* 32 seconds */
    params->own_address_type        = 0;

    memmove(params, u16_array, size_arr);
    free(u16_array);

    ConnectionDmBleSetConnectionParametersReq(params);
   
    free(params);
}

void ConnectionL2capConnectionParametersUpdateReqTestExtra(
        u8                   type,
        const bdaddr            *addr,
        u16                  min_interval,
        u16                  max_interval,
        u16                  latency,
        u16                  timeout
        )
{
    /* Stub! */ ;
}

void ConnectionDmBleConfigureLocalAddressReqTestExtra(
        u16                  local,
        u8                   type, 
        const bdaddr            *bd_addr    
        )
{
    typed_bdaddr taddr;
    taddr.type = type;
    taddr.addr = *bd_addr;

    ConnectionDmBleConfigureLocalAddressReq(
            (ble_local_addr_type)local, &taddr);
}

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
        )        
{
    /* Stub */
}

void ConnectionDmBleAcceptConnectionParUpdateResponseTestExtra(
        bool                accept_update,
        u8               type,
        const bdaddr        *bd_addr,
        u16              id,
        u16              conn_interval_min,
        u16              conn_interval_max,
        u16              conn_latency,
        u16              supervision_timeout
        )
{
    /* Stub */
}

#endif /* DISABLE_BLE */

void ConnectionGetRssiBdaddrTestExtraDefault(
        Task                    theAppTask,
        u8                   type,
        const bdaddr            *bd_addr,    
        TRANSPORT_T             transport
        )
{
    tp_bdaddr tpaddr;

    tpaddr.transport = transport;
    tpaddr.taddr.type = type;
    tpaddr.taddr.addr = *bd_addr;
    
    ConnectionGetRssiBdaddr(theAppTask, &tpaddr);
}


void ConnectionSmUserConfirmationResponseTestExtra(
        u8           type,
        const bdaddr*   bd_addr,
        u16          transport,
        bool            confirm
        )
{
    tp_bdaddr       tpaddr;

    tpaddr.transport    = (TRANSPORT_T) transport;
    tpaddr.taddr.type   = type;
    tpaddr.taddr.addr   = *bd_addr;

    ConnectionSmUserConfirmationResponse(&tpaddr, confirm);
}

void ConnectionSmUserPasskeyResponseTestExtra(
        u8           type,
        const bdaddr*   bd_addr,
        u16          transport,
        bool            cancelled,
        u32          passkey
        )
{
    tp_bdaddr       tpaddr;

    tpaddr.transport    = (TRANSPORT_T)transport;
    tpaddr.taddr.type   = type;
    tpaddr.taddr.addr   = *bd_addr;

    ConnectionSmUserPasskeyResponse(&tpaddr, cancelled, passkey);
}

void ConnectionSmSendKeypressNotificationRequestTestExtra(
        u8               type,
        const bdaddr*       bd_addr,
        u16              transport,
        u16              keypress_type
        )
{
    tp_bdaddr       tpaddr;
     
    tpaddr.transport = (TRANSPORT_T)transport;
    tpaddr.taddr.type = type;
    tpaddr.taddr.addr = *bd_addr;

    ConnectionSmSendKeypressNotificationRequest(
            &tpaddr,
            (cl_sm_keypress_type) keypress_type
            );
}


void ConnectionSmEncryptionKeyRefreshTestExtra(
        u8 type, 
        const bdaddr* bd_addr
        )
{
    typed_bdaddr    taddr;

    taddr.type      = type;
    taddr.addr      = *bd_addr;

    ConnectionSmEncryptionKeyRefresh(&taddr);
}

void ConnectionSmPinCodeResponseTestExtra(
        u8           type,
        const bdaddr*    bd_addr,
        u16          size_pin_code,
        const u8*    pin_code
        )
{
    typed_bdaddr    taddr;

    taddr.type      = type;
    taddr.addr      = *bd_addr;

    ConnectionSmPinCodeResponse(&taddr, size_pin_code, pin_code);
}

/* 
 *  B-134381
 */
void ConnectionAuthSetPriorityDeviceTestExtra(
        Task theAppTask,
        const bdaddr *bd_addr,
        bool is_priority_device
        )
{
    CL_AUTH_SET_PRIORITY_DEVICE_IND_TEST_EXTRA_T *new_msg = 
                    PanicUnlessMalloc(sizeof(CL_AUTH_SET_PRIORITY_DEVICE_IND_TEST_EXTRA_T));

    new_msg->result = ConnectionAuthSetPriorityDevice(bd_addr, is_priority_device);

    MessageSend(theAppTask, CL_AUTH_SET_PRIORITY_DEVICE_IND_TEST_EXTRA, new_msg);
}

/* 
 *  B-185975
 */

void ConnectionAuthGetPriorityDeviceStatusTestExtra(
        Task theAppTask,
        const bdaddr* bd_addr, 
        bool *is_priority_device
        )
{
    CL_AUTH_GET_PRIORITY_DEVICE_STATUS_IND_TEST_EXTRA_T *new_msg = 
                    PanicUnlessMalloc(sizeof(CL_AUTH_GET_PRIORITY_DEVICE_STATUS_IND_TEST_EXTRA_T));

    new_msg->result = ConnectionAuthGetPriorityDeviceStatus(bd_addr, &(new_msg->is_priority_device));

    MessageSend(theAppTask, CL_AUTH_GET_PRIORITY_DEVICE_STATUS_IND_TEST_EXTRA, new_msg);
}

void ConnectionAuthIsPriorityDeviceTestExtra(
        Task theAppTask,
        const bdaddr* bd_addr
        )
{
    CL_AUTH_IS_PRIORITY_DEVICE_IND_TEST_EXTRA_T *new_msg = 
                    PanicUnlessMalloc(sizeof(CL_AUTH_IS_PRIORITY_DEVICE_IND_TEST_EXTRA_T));

    new_msg->result = ConnectionAuthIsPriorityDevice(bd_addr);

    MessageSend(theAppTask, CL_AUTH_IS_PRIORITY_DEVICE_IND_TEST_EXTRA, new_msg);
}
