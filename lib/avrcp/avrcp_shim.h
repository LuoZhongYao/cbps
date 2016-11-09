/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0
*****************************************************************************/

#ifndef AVRCP_SHIM_LAYER_H
#define AVRCP_SHIM_LAYER_H

#include "avrcp.h"

#include <message.h>


typedef enum
{
    AVRCP_DUMMY_MESSAGE_TEST_EXTRA = AVRCP_MESSAGE_TOP
} AvrcpShimMessageId;


/*****************************************************************************/
void AvrcpHandleComplexMessage(Task task, MessageId id, Message message);

void AvrcpInitTestExtra(Task theAppTask, u16 dev_type);

/*****************************************************************************/
void AvrcpConnectTestNull(Task theAppTask);
void AvrcpInitBrowseTest(Task theAppTask, u16 dev_type, 
                       u16 extensions);

/*****************************************************************************/
void AvrcpPassthroughTestNull(AVRCP *avrcp, avc_subunit_type subunit_type, avc_subunit_id subunit_id, bool state, avc_operation_id opid);
void AvrcpPassthroughTestExtra(AVRCP *avrcp, avc_subunit_type subunit_type, avc_subunit_id subunit_id, bool state, avc_operation_id opid);
void AvrcpPassthroughVendorTestExtra(AVRCP *avrcp, avc_subunit_type subunit_type, avc_subunit_id subunit_id, bool state );



/*****************************************************************************/
void AvrcpVendorDependentTestExtra(AVRCP *avrcp, avc_subunit_type subunit_type, avc_subunit_id subunit_id, u8 ctype, u32 company_id);

/*****************************************************************************/
void AvrcpGetCapabilitiesResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, avrcp_capability_id caps, u16 size_caps_list, u8 *caps_list);

/*****************************************************************************/
void AvrcpListAppSettingAttributeResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 size_attributes, u8 *attributes);
void AvrcpListAppSettingValueResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 size_values, u8 *values);
void AvrcpGetCurrentAppSettingValueTestExtra(AVRCP *avrcp, u16 size_attributes, u8 *attributes);
void AvrcpGetCurrentAppSettingValueResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 size_values, u8 *values);
void AvrcpSetAppSettingValueTestExtra(AVRCP *avrcp, u16 size_attributes, u8 *attributes);
void AvrcpGetElementAttributesTestExtra(AVRCP *avrcp, u32 identifier_high, u32 identifier_low, u16 size_attributes, u8 *attributes);
void AvrcpGetElementAttributesResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 number_of_attributes, u16 size_attributes, u8 *attributes);
void AvrcpGetElementAttributesFragmentedResponseTestExtra(
                                      AVRCP               *avrcp,
                                      avrcp_response_type response,
                                      u16              number_of_attributes); 
void AvrcpEventPlayerAppSettingChangedResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 size_attributes, u8 *attributes);
void AvrcpInformDisplayableCharacterSetTestExtra(AVRCP *avrcp, u16 size_attributes, u8 *attributes);
void AvrcpGetAppSettingAttributeTextTestExtra(AVRCP *avrcp, u16 size_attributes, u8 *attributes);
void AvrcpGetAppSettingAttributeTextResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 number_of_attributes, u16 size_attributes, u8 *attributes);
void AvrcpGetAppSettingValueTextTestExtra(AVRCP *avrcp, u16 attribute_id, u16 size_values, u8 *values);
void AvrcpGetAppSettingValueTextResponseTestExtra(AVRCP *avrcp, avrcp_response_type response, u16 number_of_values, u16 size_values, u8 *values);

void AvrcpBrowseGetFolderItemsTest(AVRCP                *avrcp,  
                                   avrcp_browse_scope   scope);

void AvrcpBrowseGetItemAttributesTest(AVRCP*               avrcp, 
                                      avrcp_browse_scope   scope,  
                                      u16               uid_counter);  

void AvrcpBrowseSearchTest( AVRCP*          avrcp, 
                            u16          str_length, 
                            u8*          string); 

void AvrcpBrowseSetPlayerResponseTest(AVRCP*               avrcp, 
                                      avrcp_response_type  response,
                                      u16               uid_counter,   
                                      u32               num_items);

void AvrcpBrowseGetFolderItemsResponseTest(AVRCP*               avrcp,
                                           avrcp_response_type  response, 
                                           u16               uid_counter,
                                           u8                item_type);

void AvrcpBrowseGetItemAttributesResponseTest(  AVRCP*               avrcp, 
                                                avrcp_response_type  response);

void AvrcpBrowseChangePathTest(AVRCP*                  avrcp,  
                               u16                  uid_counter,
                               avrcp_browse_direction  direction);

void AvrcpPlayItemTest( AVRCP*              avrcp, 
                        avrcp_browse_scope  scope,    
                        u16              uid_counter);

void AvrcpAddToNowPlayingTest( AVRCP*              avrcp, 
                               avrcp_browse_scope  scope,    
                               u16            uid_counter);

#endif
