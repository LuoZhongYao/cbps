/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0


FILE NAME
    avrcp_signal_unit_info.h
    
DESCRIPTION
    
*/

#ifndef AVRCP_SIGNAL_UNIT_INFO_H_
#define AVRCP_SIGNAL_UNIT_INFO_H_

#include "avrcp_common.h"

#ifndef AVRCP_TG_ONLY_LIB /* Disable CT for TG only lib */
/****************************************************************************
NAME    
    avrcpSendUnitInfoCfmToClient

DESCRIPTION
    This function creates a AVRCP_UNITINFO_CFM message and sends it to 
    the client task.
*/
void avrcpSendUnitInfoCfmToClient(AVRCP *avrcp, avrcp_status_code status,
                                 u16 unit_type, u16 unit, u32 company);


/****************************************************************************
NAME    
    avrcpHandleInternalUnitInfoReq

DESCRIPTION
    This function internally handles unit info message request.
*/
void avrcpHandleInternalUnitInfoReq(AVRCP *avrcp);

/****************************************************************************
NAME    
    avrcpHandleUnitInfoResponse

DESCRIPTION
    This function internally handles unit info response received from 
    a remote device.
*/
void avrcpHandleUnitInfoResponse(AVRCP *avrcp, 
                           const u8 *ptr, 
                                 u16 packet_size);

/****************************************************************************
NAME    
    avrcpSendSubunitInfoCfmToClient

DESCRIPTION
    This function creates a AVRCP_SUBUNITINFO_CFM message and sends it to 
    the client task.
*/
void avrcpSendSubunitInfoCfmToClient(AVRCP            *avrcp, 
                                     avrcp_status_code status, 
                                     u8             page, 
                               const u8             *page_data);

/****************************************************************************
NAME    
    avrcpHandleInternalSubUnitInfoReq

DESCRIPTION
    This function internally handles subunit info message request.
*/
void avrcpHandleInternalSubUnitInfoReq(AVRCP *avrcp, 
                                 const AVRCP_INTERNAL_SUBUNITINFO_REQ_T *req);


/****************************************************************************
NAME    
    avrcpHandleSubUnitInfoResponse

DESCRIPTION
    This function internally handles subunit info response received 
    from a remote device.
*/
void avrcpHandleSubUnitInfoResponse(AVRCP *avrcp, 
                              const u8 *ptr, 
                                    u16 packet_size);
#endif /* !AVRCP_TG_ONLY_LIB */

#ifndef AVRCP_CT_ONLY_LIB /* Disable TG for CT only lib */
/****************************************************************************
NAME    
    avrcpHandleInternalUnitInfoRes

DESCRIPTION
    This function internally handles unit info message result.
*/
void avrcpHandleInternalUnitInfoRes(AVRCP *avrcp, 
                              const AVRCP_INTERNAL_UNITINFO_RES_T *req);


/****************************************************************************
NAME    
    avrcpHandleUnitInfoCommand

DESCRIPTION
    This function internally handles unit info command received from
    a remote device.
*/
void avrcpHandleUnitInfoCommand(AVRCP *avrcp, 
                          const u8 *ptr, 
                                u16 packet_size);


/****************************************************************************
NAME    
    avrcpHandleInternalSubUnitInfoRes

DESCRIPTION
    This function internally handles subunit info message result.
*/
void avrcpHandleInternalSubUnitInfoRes(AVRCP *avrcp, 
                                 const AVRCP_INTERNAL_SUBUNITINFO_RES_T *res);


/****************************************************************************
NAME    
    avrcpHandleSubUnitInfoCommand

DESCRIPTION
    This function internally handles subunit info command received
     from a remote device.
*/
void avrcpHandleSubUnitInfoCommand(AVRCP *avrcp, 
                             const u8 *ptr, 
                                   u16 packet_size);



#endif /* !AVRCP_CT_ONLY_LIB*/

#endif /* AVRCP_SIGNAL_UNIT_INFO_H_ */

