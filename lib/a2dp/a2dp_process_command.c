/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_process_command.c

DESCRIPTION

NOTES

*/



/****************************************************************************
    Header files
*/

#include "a2dp_process_command.h"
#include "a2dp_command_handler.h"
#include "a2dp_data_block.h"
#include "a2dp_packet_handler.h"
#include "a2dp_caps_parse.h"
#include "a2dp_api.h"
#include "a2dp_l2cap_handler.h"
#include "a2dp_codec_handler.h"

#include <print.h>
#include <stdlib.h>
#include <string.h>
#include <sink.h>

#define SEP_INDEX_INVALID (0xFF)


static const u8 media_transport_caps[] = { AVDTP_SERVICE_MEDIA_TRANSPORT, 0x00 };
static const u8 content_protection_caps[] = { AVDTP_SERVICE_CONTENT_PROTECTION, 0x02, AVDTP_CP_TYPE_SCMS_LSB, AVDTP_CP_TYPE_SCMS_MSB };
static const u8 delay_reporting_caps[] = { AVDTP_SERVICE_DELAY_REPORTING, 0x00 };


static u8 getLocalSeid (remote_device *device)
{
    sep_data_type *current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list );
    
    if (current_sep)
    {
        return current_sep->sep_config->seid;
    }

    return 0;
}

static u8 getSepIndexBySeid (remote_device *device, u16 seid)
{
    u8 index;
    u8 max_index = blockGetSize( device->device_id, data_block_sep_list ) / sizeof( sep_data_type );
    sep_data_type *pSeps = (sep_data_type *)PanicNull( blockGetBase( device->device_id, data_block_sep_list ) );

    for ( index=0; index<max_index; index++ )
    {
        if (pSeps->sep_config->seid == seid)
        {
            return index;
        }
        
        pSeps++;
    }

    return SEP_INDEX_INVALID;
}

static void updateConfiguredServiceCaps (remote_device *device, u8 local_sep_index, const u8 *new_service_caps, u16 new_service_caps_size)
{
    const u8 *media_codec_caps;
    u8 *configured_caps;
    u16 media_codec_caps_size;
    u16 service_caps_size;
    u16 configured_caps_size;
    bool content_protection_supported = FALSE;
    bool delay_reporting_supported = FALSE;
    
    /* Obtain location and size of currently configured service caps (if any) */
    configured_caps = blockGetBase(device->device_id, data_block_configured_service_caps);
    configured_caps_size = blockGetSize(device->device_id, data_block_configured_service_caps);
    service_caps_size = sizeof(media_transport_caps);
    
    /* Determine location and size of the caps we wish to retain (media codec) */
    media_codec_caps = new_service_caps;
    media_codec_caps_size = new_service_caps_size;
    if ( !a2dpFindCodecSpecificInformation(&media_codec_caps, &media_codec_caps_size) )
    {   /* Caps not being updated, get current caps (if any) */
        media_codec_caps = configured_caps;
        media_codec_caps_size = configured_caps_size;
        if ( !a2dpFindCodecSpecificInformation(&media_codec_caps, &media_codec_caps_size) )
        {   /* No currently existing media codec caps */
            media_codec_caps = NULL;
        }
    }
    media_codec_caps_size = (media_codec_caps == NULL)? 0 : 2 + media_codec_caps[1];
    service_caps_size += media_codec_caps_size;
    
    /* Determine location and size of the caps we wish to retain (content protection) */
    if (!a2dpIsServiceSupported(AVDTP_SERVICE_CONTENT_PROTECTION, new_service_caps, new_service_caps_size))
    {   /* Caps not being updated, get current caps (if any) */
        if (a2dpIsServiceSupported(AVDTP_SERVICE_CONTENT_PROTECTION, configured_caps, configured_caps_size))
        {
            content_protection_supported = TRUE;
            service_caps_size += sizeof(content_protection_caps);
        }
    }
    else
    {   /* Caps being updated, do we support Content Protection locally? */
        const sep_config_type *local_sep_config = ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, local_sep_index ) ))->sep_config;
        
        if (a2dpIsServiceSupported(AVDTP_SERVICE_CONTENT_PROTECTION, local_sep_config->caps, local_sep_config->size_caps))
        {   /* CP supported locally, include in configured caps */
            content_protection_supported = TRUE;
            service_caps_size += sizeof(content_protection_caps);
        }
    }
    
    /* Determine location and size of the caps we wish to retain (content protection) */
    if (!a2dpIsServiceSupported(AVDTP_SERVICE_DELAY_REPORTING, new_service_caps, new_service_caps_size))
    {   /* Caps not being updated, get current caps (if any) */
        if (a2dpIsServiceSupported(AVDTP_SERVICE_DELAY_REPORTING, configured_caps, configured_caps_size))
        {
            delay_reporting_supported = TRUE;
            service_caps_size += sizeof(delay_reporting_caps);
        }
    }
    else
    {   /* Caps being updated, do we support Content Protection locally? */
        const sep_config_type *local_sep_config = ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, local_sep_index ) ))->sep_config;
        
        if (a2dpIsServiceSupported(AVDTP_SERVICE_DELAY_REPORTING, local_sep_config->caps, local_sep_config->size_caps))
        {   /* Delay Reporting supported locally, include in configured caps */
            delay_reporting_supported = TRUE;
            service_caps_size += sizeof(delay_reporting_caps);
        }
    }
    
    if (service_caps_size)
    {
        u8 *service_caps;
        u8 *write_ptr;
        
        /* Allocate temporary storage area */    
        write_ptr = service_caps = (u8 *)PanicNull( malloc(service_caps_size) );
        
        /* Fill temporary storage area with most current information */
        memmove(write_ptr, media_transport_caps, sizeof(media_transport_caps));
        write_ptr +=  sizeof(media_transport_caps);
        
        if (media_codec_caps)
        {
            memmove(write_ptr, media_codec_caps, media_codec_caps_size);
            write_ptr+= media_codec_caps_size;
        }
        
        if (content_protection_supported)
        {
            memmove(write_ptr, content_protection_caps, sizeof(content_protection_caps));
            write_ptr += sizeof(content_protection_caps);
        }
        
        if (delay_reporting_supported)
        {
            memmove(write_ptr, delay_reporting_caps, sizeof(delay_reporting_caps));
            write_ptr += sizeof(delay_reporting_caps);
        }
        
        /* Update configured service caps data block */
        blockRemove( device->device_id, data_block_configured_service_caps );
        configured_caps = (u8 *)PanicNull( blockAdd( device->device_id, data_block_configured_service_caps, service_caps_size, sizeof(u8) ));
        memmove(configured_caps, service_caps, service_caps_size);
        
        /* Release the temporary storage area */
        free(service_caps);
    }
}


static u16 getFilteredServiceCaps (u8 *filtered_caps, u8 filter, const u8 *full_caps, u16 size_full_caps)
{
    u16 size_filtered_caps = 0;
    
    while (size_full_caps)
    {
        u16 size = full_caps[1] + 2;
        
        if (filter!=full_caps[0])
        {   /* Copy this service category as it is not being filtered out */
            size_filtered_caps += size;
            if (filtered_caps)
            {
                memcpy(filtered_caps, full_caps, size);
                filtered_caps += size;
            }
        }
        
        full_caps += size;
        size_full_caps -= size;
    }
    
    return size_filtered_caps;
}


/****************************************************************************
NAME
    a2dpSetSepAvailable

DESCRIPTION
    Marks a SEP as either available/unavailable.

RETURNS
    TRUE if SEP could be found, FALSE otherwise
*/
bool a2dpSetSepAvailable (remote_device *device, u8 seid, bool available)
{
    u8 sep_index = getSepIndexBySeid(device, seid);

    if (sep_index != SEP_INDEX_INVALID)
    {
        sep_data_type *sep_ptr = (sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) );
        
        if (!available)
        {
            sep_ptr->in_use |= A2DP_SEP_UNAVAILABLE;
        }
        else
        {
            sep_ptr->in_use &= ~A2DP_SEP_UNAVAILABLE;
        }
        
        return TRUE;
    }
    
    return FALSE;
}


/****************************************************************************
NAME
    a2dpGetSepAvailability

DESCRIPTION
    Obtains the availability of a SEP.

RETURNS
    Availability state
*/
a2dp_sep_status a2dpGetSepAvailability (remote_device *device, u8 seid)
{
    u8 sep_index = getSepIndexBySeid(device, seid);

    if (sep_index != SEP_INDEX_INVALID)
    {
        sep_data_type *sep_ptr = (sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) );
        return sep_ptr->in_use;
    }
    
    return A2DP_SEP_UNAVAILABLE;
}


/****************************************************************************
NAME
    processDiscover

DESCRIPTION
    Process an incoming discover request.

RETURNS
    void
*/
u16 a2dpProcessDiscoverCommand (remote_device *device, u16 *payload_size)
{
    Sink sink = device->signal_conn.connection.active.sink;
    sep_data_type *sep_ptr = (sep_data_type *)PanicNull( blockGetBase( device->device_id, data_block_sep_list ) );
    u8 sep_cnt = blockGetSize( device->device_id, data_block_sep_list ) / sizeof(sep_data_type);
    u8 *payload;
    u8 i;
    
    *payload_size = 0;
    for (i=0; i<sep_cnt; i++)
    {
        if ( !(sep_ptr[i].in_use & A2DP_SEP_UNAVAILABLE) )
        {
            *payload_size+=2;
        }
    }
    
    if (!(*payload_size) || (payload = a2dpGrabSink(sink, *payload_size)) == NULL)
    {
        return avdtp_bad_state;
    }
    
    while (sep_cnt--)
    {
        const sep_config_type *sep_config = sep_ptr->sep_config;

        if ((sep_config->role != a2dp_role_undefined) && !(sep_ptr->in_use & A2DP_SEP_UNAVAILABLE))
        {
            if (sep_ptr->in_use)
            {
                *payload++ = (sep_config->seid << 2) | 0x02;
            }
            else
            {
                *payload++ = sep_config->seid << 2;
            }
            
            *payload++ = (u8) ((sep_config->media_type << 4) | ((sep_config->role==a2dp_sink) ? (0x01<<3) :0x00));
            
            /**payload_size += 2;*/
        }
        
        sep_ptr++;
    }

    return avdtp_ok;
}

/****************************************************************************
NAME
    processGetCapabilitiesCommand

DESCRIPTION
    Process an incoming Get_Capabilities request

RETURNS
    void
*/
u16 a2dpProcessGetCapabilitiesCommand (remote_device *device, u16 *payload_size)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    Sink sink = device->signal_conn.connection.active.sink;
    u8 *payload;

    *payload_size = 0;
    
    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else
    {
        const sep_config_type *sep_config = ((const sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;
        u16 size_caps = getFilteredServiceCaps( NULL, AVDTP_SERVICE_DELAY_REPORTING, sep_config->caps, sep_config->size_caps);
        
        if ((payload = a2dpGrabSink(sink, size_caps)) == NULL)
        {
            return avdtp_bad_state;
        }
    
        *payload_size = size_caps;
        getFilteredServiceCaps( payload, AVDTP_SERVICE_DELAY_REPORTING, sep_config->caps, sep_config->size_caps);
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processSetConfigurationCommand

DESCRIPTION
    Process an incoming Set_Configuration request

RETURNS
    void
*/
u16 a2dpProcessSetConfigurationCommand (remote_device *device, u16 *payload_size)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    u16 packet_size = device->signal_conn.connection.active.received_packet_length;
    u8 unsupported_service;
    u8 error_cat, error_code;

    *payload_size = 0;
    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else 
    {
        sep_data_type *sep_ptr = (sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) );
        const sep_config_type *sep_config = sep_ptr->sep_config;
        const u8 *codec_data = NULL;
        
        if ((sep_config->role == a2dp_source) && (device->signal_conn.status.stream_state == avdtp_stream_configuring))
        {   /* Remote device is attempting to configure a source SEP while we are currently configuring - give priority to source */
            return avdtp_bad_state;
        }
        else if (sep_ptr->in_use)
        {   /* SEP is already in use - reject (service capabilities were not the problem)*/
            return avdtp_sep_in_use;
        }
        else if (!a2dpValidateServiceCaps(&ptr[4], packet_size-4, FALSE, FALSE, FALSE, &error_cat, &error_code))
        {   /* bad caps - reject */
            return (error_cat << 8) | error_code;
        }
        else if (!a2dpAreServicesCategoriesCompatible(sep_config->caps, sep_config->size_caps, &ptr[4], packet_size-4, &unsupported_service))
        {   /* Check that configuration only asks for services the local SEP supports set config does not match our caps - reject */
            return (unsupported_service <<8) | avdtp_unsupported_configuration;
        }
        else if ((codec_data = a2dpFindMatchingCodecSpecificInformation(sep_config->caps, &ptr[4], 0)) == NULL)
        {   /*  Check the codec specific attributes are compatible set config does not match our caps - reject */
            return (AVDTP_SERVICE_MEDIA_CODEC << 8) | avdtp_unsupported_configuration;
        }
        else
        {
            /* Set the index to the current SEID data */
            blockSetCurrent( device->device_id, data_block_sep_list, sep_index );
            device->local_sep.seid = sep_config->seid;
            device->local_sep.role = sep_config->role;
            device->local_sep.codec = codec_data[AVDTP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];
            
            /* Mark this SEP as in use */
            sep_ptr->in_use |= A2DP_SEP_IN_USE;
    
            /* Store remote SEID */
            device->remote_sep.seid = (ptr[3] >> 2) & 0x3f;
            device->remote_sep.role = (sep_config->role==a2dp_sink)?a2dp_source:a2dp_sink;  /* Remote role must be opposite of our own */
            device->remote_sep.codec = codec_data[AVDTP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];
            
            /* Set current index for list of discovered remote seids */
            /* TODO */

            updateConfiguredServiceCaps(device, sep_index, &ptr[4], packet_size-4);
            
            return avdtp_ok;
        }
    }    
}


/****************************************************************************
NAME
    processGetConfigurationCommand

DESCRIPTION
    Process an incoming Get_Capabilities request.

RETURNS
    void
*/
u16 a2dpProcessGetConfigurationCommand (remote_device *device, u16 *payload_size)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    Sink sink = device->signal_conn.connection.active.sink;
    
    *payload_size = 0;
    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else
    {
        u8 *payload;
                
        *payload_size = blockGetSize( device->device_id, data_block_configured_service_caps );
        /* build header with transaction label from request. */
        if ((payload = a2dpGrabSink(sink, *payload_size)) == NULL)
        {
            return avdtp_bad_state;
        }

        /* copy in the agreed configuration for this SEP.
           Note that this only returns the last SET_CONFIG or RECONFIGURE
           parameters and not the global configuration - is this ok? */
        memmove( payload, PanicNull( blockGetBase( device->device_id, data_block_configured_service_caps ) ), *payload_size );
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processReconfigure

DESCRIPTION
    Process an incoming reconfigure request.

RETURNS
    void
*/
u16 a2dpProcessReconfigureCommand (remote_device *device, u16 *payload_size)
{
    const u8 *reconfig_caps = device->signal_conn.connection.active.received_packet + 3;
    u16 reconfig_caps_size = device->signal_conn.connection.active.received_packet_length - 3;
    u8 error_cat, error_code;
    u8 unsupported_service;
    u8 sep_index = getSepIndexBySeid(device, (device->signal_conn.connection.active.received_packet[2]>>2) & 0x3f);

    *payload_size = 0;
    /*
        AVDTP test TP/SIG/SMG/BI-14-C is very pedantic about the order in which
        we should return errors, even though the spec does not state a validation
        procedure.  This code is therefore more verbose than it needs to be
        in order to generate the correct errors in the correct order.
    */
    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else
    {
        const sep_config_type *sep_config = ((const sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;
    
        if (!a2dpValidateServiceCaps(reconfig_caps, reconfig_caps_size, FALSE, TRUE, FALSE, &error_cat, &error_code))
        {   /* bad caps - reject */
            return (error_cat << 8) | error_code;
        }
        else if (getLocalSeid(device) != ((device->signal_conn.connection.active.received_packet[2]>>2) & 0x3f))
        {   /* SEP is already in use - reject */
            return avdtp_sep_not_in_use;
        }
        /* check that the service caps are valid for reconfigure */
        else if (!a2dpValidateServiceCaps(reconfig_caps, reconfig_caps_size, TRUE, FALSE, FALSE, &error_cat, &error_code))
        {   /*  bad caps - reject */
            return (error_cat << 8) | error_code;
        }
        else if (!a2dpAreServicesCategoriesCompatible(sep_config->caps, sep_config->size_caps, reconfig_caps, reconfig_caps_size, &unsupported_service))
        {   /* set config does not match our caps - reject */
            return (unsupported_service << 8) | avdtp_unsupported_configuration;
        }
        else if (a2dpFindMatchingCodecSpecificInformation(sep_config->caps, reconfig_caps, 0) == NULL)
        {   /* requested codec is not compatible with our caps */
            return (AVDTP_SERVICE_MEDIA_CODEC << 8) | avdtp_invalid_capabilities;
        }
        else
        {
            updateConfiguredServiceCaps(device, sep_index, reconfig_caps, reconfig_caps_size);
            
            return avdtp_ok;
        }
    }
}


/****************************************************************************
NAME
    processGetAllCapabilitiesCommand

DESCRIPTION
    Process an incoming Get_All_Capabilities request

RETURNS
    void
*/
u16 a2dpProcessGetAllCapabilitiesCommand (remote_device *device, u16 *payload_size)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    Sink sink = device->signal_conn.connection.active.sink;
    u8 *payload;

    *payload_size = 0;
    
    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else
    {
        const sep_config_type *sep_config = ((const sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;
        u16 size_caps = getFilteredServiceCaps( NULL, 0, sep_config->caps, sep_config->size_caps);
        
        if ((payload = a2dpGrabSink(sink, size_caps)) == NULL)
        {
            return avdtp_bad_state;
        }
    
        *payload_size = size_caps;
        getFilteredServiceCaps( payload, 0, sep_config->caps, sep_config->size_caps);
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processDelayReport

DESCRIPTION
    Process an incoming Delay Report notification.

RETURNS
    void
*/
u16 a2dpProcessDelayReportCommand (remote_device *device)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);

    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else if (device->local_sep.role != a2dp_source)
    {   /* AVDTP_DELAY_REPORT_CMD can only be issued for endpoints configured as a SRC */
        return avdtp_bad_state;
    }
    else
    {
        device->media_conn[0].status.delay = ((u16)ptr[3]<<8) | ptr[4];
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processOpen

DESCRIPTION
    Process an incoming Open request.

RETURNS
    void
*/
u16 a2dpProcessOpenCommand (remote_device *device)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);

    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else
    {
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processStart

DESCRIPTION
    Process a Start request.

RETURNS
    void
*/
u16 a2dpProcessStartCommand (remote_device *device)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet + 2;
    u16 seids = device->signal_conn.connection.active.received_packet_length - 2;

    while (seids--)
    {
        u8 seid = (*ptr++ >> 2) & 0x3f;
        u8 sep_index = getSepIndexBySeid(device, seid);

        if (sep_index == SEP_INDEX_INVALID)
        {
            return (seid << 10) | avdtp_bad_acp_seid;
        }
        else if (getLocalSeid(device) != seid)
        {
            return (seid << 10) | avdtp_bad_state;
        }
    }

    /* All SEIDs valid, accept */
    return avdtp_ok;
}


/****************************************************************************
NAME
    processClose

DESCRIPTION
    Process a Close request.

RETURNS
    void
*/
u16 a2dpProcessCloseCommand(remote_device *device)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet;
    u8 sep_index = getSepIndexBySeid(device, (ptr[2]>>2) & 0x3f);
    
    if (sep_index == SEP_INDEX_INVALID)
    {
        return avdtp_bad_acp_seid;
    }
    else if (getLocalSeid(device) != ((ptr[2]>>2) & 0x3f))
    {
        return avdtp_bad_state;
    }
    else
    {
        /* Mark the SEP as no longer being in use */
        ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->in_use &= ~A2DP_SEP_IN_USE;
        
        /* Reset local and remote SEIDs */
        blockSetCurrent( device->device_id, data_block_sep_list, 0 );
        device->remote_sep.seid = 0;
        
        /* Remove stream specific data */
        blockRemove( device->device_id, data_block_configured_service_caps );
        blockRemove( device->device_id, data_block_list_discovered_remote_seids );
        
        return avdtp_ok;
    }
}


/****************************************************************************
NAME
    processSuspend

DESCRIPTION
    Process a suspend request.

RETURNS
    void
*/
u16 a2dpProcessSuspendCommand (remote_device *device)
{
    const u8 *ptr = device->signal_conn.connection.active.received_packet + 2;
    u16 seids = device->signal_conn.connection.active.received_packet_length - 2;
    
    while (seids--)
    {
        u8 seid = (*ptr++ >> 2) & 0x3f;
        u8 sep_index = getSepIndexBySeid(device, seid);

        if (sep_index == SEP_INDEX_INVALID)
        {
            return (seid << 10) | avdtp_bad_acp_seid;
        }
        else if (getLocalSeid(device) != seid)
        {
            return (seid << 10) | avdtp_bad_state;
        }
    }

    /* all SEIDs valid, accept */
    return avdtp_ok;
}


/****************************************************************************
NAME
    processAbort

DESCRIPTION
    Process an Abort request.

RETURNS
    void
*/
bool a2dpProcessAbortCommand(remote_device *device)
{
    const u8* ptr = device->signal_conn.connection.active.received_packet;
    sep_data_type *current_sep;

    if ( (current_sep = (sep_data_type *)blockGetCurrent( device->device_id, data_block_sep_list )) != NULL )
    {
        if (((ptr[2]>>2) & 0x3f) == current_sep->sep_config->seid)
        {
            return TRUE;
        }
    }
    
    return FALSE;
}


bool a2dpProcessDiscoverResponse(remote_device *device)
{
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetCurrent( device->device_id, data_block_sep_list ) ))->sep_config;
    const u8* ptr = (const u8 *)PanicNull(device->signal_conn.connection.active.received_packet);
    u16 packet_size = device->signal_conn.connection.active.received_packet_length;
    u8 sep_style;
    u8 discovered_remote_seps;
    u32 store_seps;
    u32 sep_mask;
    u16 i;
    sep_info *remote_sep;

    /* clear record of discovered SEPs */
    blockRemove( device->device_id, data_block_list_discovered_remote_seids );

    ptr += 2;

    sep_style = (u8)sep_config->media_type << 4;
    /*sep_style |= ((u8)sep_config->role ^ 0x01) << 3;*/  /* Note: Making an assumption about an enumerated type */
    discovered_remote_seps = 0;
    store_seps = 0;
    sep_mask = 0x0001;
    for (i=0; i<(packet_size-2) && sep_mask; i++)  /* Note: This algorithm will limit discovery to the first 16 remote seids */
    {
        /* Check that the SEP is not in use */
        if ( (ptr[i++] & 2)==0 )
        {
            /* Check that the media type matches and also that the SEP's role is the opposite */
            if ( sep_style == (ptr[i] & 0xF0) )
            {
                discovered_remote_seps++;
                store_seps |= sep_mask;    /* Record that this is a seid we are interested in */
            }
            sep_mask <<= 1;
        }
    }
    
    if (discovered_remote_seps)
    {
        remote_sep = (sep_info *)PanicNull( blockAdd( device->device_id, data_block_list_discovered_remote_seids, discovered_remote_seps, sizeof(sep_info) ) );
        
        /* Re-iterate through data and... */
        while( store_seps )
        {
            if ( store_seps & 0x0001 )
            {   /* ...store seids we are interested in */
                remote_sep->seid = ptr[0] >> 2;
                remote_sep->role = ((ptr[1] >> 3) & 0x01) ? a2dp_sink : a2dp_source;
                remote_sep->codec = AVDTP_MEDIA_CODEC_UNDEFINED;
                remote_sep++;
            }
            store_seps >>= 1;
            ptr += 2;
        }
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



bool a2dpProcessGetCapabilitiesResponse(remote_device *device)
{
    const u8 *remote_caps = (const u8 *)PanicNull(device->signal_conn.connection.active.received_packet) + 2;
    u16 remote_caps_size = device->signal_conn.connection.active.received_packet_length - 2;
    u8 error_cat, error_code;
    const u8 *remote_codec;
    u8 sep_index = getSepIndexBySeid(device, device->local_sep.seid);
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;

    if ( !a2dpValidateServiceCaps(remote_caps, remote_caps_size, FALSE, FALSE, TRUE, &error_cat, &error_code) )
    {   /* Returned capabilities are out of spec */
        return FALSE;
    }
    
    remote_codec = remote_caps;
    if ( a2dpFindCodecSpecificInformation(&remote_codec, 0) )
    {   /* Update remote sep list with media codec type (slightly incohesive) */
        sep_info *remote_sep = (sep_info *)PanicNull( blockGetCurrent(device->device_id, data_block_list_discovered_remote_seids) );
        remote_sep->codec = remote_codec[AVDTP_SERVICE_CAPS_MEDIA_CODEC_TYPE_OFFSET];
        
        /* Update local copy of remote sep info (Definately incohesive) */
        device->remote_sep = *remote_sep;
    }
    
    /* Codec Configuration. */
    if ( a2dpFindMatchingCodecSpecificInformation(sep_config->caps, remote_codec, TRUE) == NULL)
    {   /* Current local and remote codecs are not compatible - either codec type and/or capabilities */
        return FALSE;
    }

    /* Will update the Configured Service caps to actually contain the *complete capabilities* of the remote codec */
    /* This is later refined to usable configuration by a2dpSelectConfigurationParameters()                        */
    updateConfiguredServiceCaps(device, sep_index, remote_caps, remote_caps_size);
    
    return TRUE;
}

u16 a2dpSelectConfigurationParameters(remote_device *device)
{
    u8 sep_index = getSepIndexBySeid(device, device->local_sep.seid);
    const sep_config_type *sep_config = ((sep_data_type *)PanicNull( blockGetIndexed( device->device_id, data_block_sep_list, sep_index ) ))->sep_config;
    
    blockSetCurrent( device->device_id, data_block_sep_list, sep_index );
    
    if (sep_config->library_selects_settings)
    {
        u8 *remote_service_caps = (u8 *)PanicNull( blockGetBase(device->device_id, data_block_configured_service_caps) );
        
        if (!a2dpSelectOptimalCodecSettings(device, remote_service_caps))
        {
            return CONFIGURATION_NOT_SELECTED;
        }
        else
        {
            return CONFIGURATION_SELECTED;
        }
    }
    else
    {
        return CONFIGURATION_BY_CLIENT;
    }
}


bool a2dpProcessCodecConfigureResponse(remote_device *device, u8 local_seid, const u8 *codec_caps, u16 size_codec_caps)
{
    u8 sep_index = getSepIndexBySeid(device, local_seid);
    
    if ((sep_index == SEP_INDEX_INVALID) || (local_seid != device->local_sep.seid))
    {
        return FALSE;
    }
    
    updateConfiguredServiceCaps(device, sep_index, codec_caps, size_codec_caps);
    
    return TRUE;
}


void a2dpProcessReconfigureResponse(remote_device *device)
{
    u8 sep_index = getSepIndexBySeid(device, device->local_sep.seid);
    
    updateConfiguredServiceCaps(device, sep_index, device->reconfig_caps, device->reconfig_caps_size);
    
    /* No longer need to keep track of app supplied data */
    device->reconfig_caps = NULL;
    device->reconfig_caps_size = 0;
}

bool a2dpProcessGetAllCapabilitiesResponse(remote_device *device)
{
    return a2dpProcessGetCapabilitiesResponse(device);
}

