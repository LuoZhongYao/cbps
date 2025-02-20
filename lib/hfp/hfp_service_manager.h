/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    hfp_link_manager.h
    
DESCRIPTION
    The link manager provides commonly used functionality for accessing links
    in hfp_task_data.
*/

#ifndef HFP_SERVICE_MANAGER_H_
#define HFP_SERVICE_MANAGER_H_

/* Iterate over all services */
#define for_all_services(service)      for(service = theHfp->services; service < theHfp->top; service++)
/* The address of the top service */
#define HFP_SERVICE_TOP (theHfp->top - 1)
/* Register a service's channel number with RFCOMM */
#define hfpServiceChannelRegister(service) ConnectionRfcommAllocateChannel(&theHfp->task, service->rfc_server_channel)

/****************************************************************************
NAME
    hfpServicesInit

DESCRIPTION
    Set up theHfp->services structure with profiles from app configuration
    and corresponding default RFCOMM channels.
*/
void hfpServicesInit(hfp_profile profile, bool multipoint);


/****************************************************************************
NAME    
    hfpGetServiceFromChannel

DESCRIPTION
    Get service data corresponding to a given RFC server channel

RETURNS
    Pointer to the corresponding hfp_service_data if successful. 
    Otherwise NULL.
*/
hfp_service_data* hfpGetServiceFromChannel(u8 channel);


/****************************************************************************
NAME    
    hfpGetServiceFromHandle

DESCRIPTION
    Get service data corresponding to a given service record handle

RETURNS
    Pointer to the corresponding hfp_service_data if successful. 
    Otherwise NULL.
*/
hfp_service_data* hfpGetServiceFromHandle(u32 service_handle);


/****************************************************************************
NAME    
    hfpGetServiceFromProfile

DESCRIPTION
    Get service data for any service that supports given profile

RETURNS
    Pointer to the corresponding hfp_service_data if successful. 
    Otherwise NULL.
*/
hfp_service_data* hfpGetServiceFromProfile(hfp_profile profile);

/****************************************************************************
NAME    
    hfpGetDisconnectedServiceFromProfile

DESCRIPTION
    Get service data for any service that supports given profile and is not 
    in use by a link in hfp_slc_complete state

RETURNS
    Pointer to the corresponding hfp_service_data if successful. 
    Otherwise NULL.
*/
hfp_service_data* hfpGetDisconnectedServiceFromProfile(hfp_profile profile);


/****************************************************************************
NAME    
    hfpGetVisibleServiceFromProfile

DESCRIPTION
    Get service data for any service that supports given profile and has a 
    visible SDP record

RETURNS
    Pointer to the corresponding hfp_service_data if successful. 
    Otherwise NULL.
*/
hfp_service_data* hfpGetVisibleServiceFromProfile(hfp_profile profile);

#endif /* HFP_SERVICE_MANAGER_H_ */
