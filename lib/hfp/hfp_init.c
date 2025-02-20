/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    init.c

DESCRIPTION
    This file contains the initialisation code for the Hfp profile library.

NOTES

*/
/*lint -e655 */

/****************************************************************************
    Header files
*/
#include "hfp.h"
#include "hfp_private.h"
#include "hfp_service_manager.h"
#include "hfp_link_manager.h"
#include "hfp_common.h"
#include "hfp_rfc.h"
#include "hfp_sdp.h"
#include "hfp_init.h"
#include "hfp_profile_handler.h"
#include "hfp_wbs_handler.h"

#include <panic.h>
#include <print.h>
#include <string.h>

/* The main HFP data struct */
hfp_task_data* theHfp;

/****************************************************************************
NAME
    hfpInitCfmToApp

DESCRIPTION
    Send an init cfm message to the application 
*/
static void hfpInitCfmToApp(hfp_init_status status)
{
    MAKE_HFP_MESSAGE(HFP_INIT_CFM);
    message->status = status;
    MessageSend(theHfp->clientTask, HFP_INIT_CFM, message);

    /* If the initialisation failed, free the allocated task */
    if (status > hfp_init_reinit_fail)
        free(theHfp);
}


/****************************************************************************
NAME
    HfpInit

DESCRIPTION
    

MESSAGE RETURNED
    HFP_INIT_CFM

RETURNS
    void
*/
void HfpInit(Task theAppTask, const hfp_init_params *config, const char* extra_indicators)
{
    if(theHfp)
    {
        hfpInitCfmToApp(hfp_init_reinit_fail);
        return;
    }
    
    /* Check the app has passed in a valid pointer. */
    if (!config)
    {
        HFP_DEBUG(("Config parameters not passed in\n"));
    }
    else
    {
        u8             size_hfp_data;
        hfp_task_data*    lHfp;
        hfp_link_data*    link;
        hfp_profile       profile  = config->supported_profile;
        
        /* Calculate number of links and services */
        u8 num_links    = (config->multipoint ? 2 : 1);
        u8 num_services = 0;
        
        if(supportedProfileIsHfp(profile))
            num_services += num_links;
        
        if(supportedProfileIsHsp(profile))
            num_services += num_links;
        
        /* Calculate overall memory requirement */
        size_hfp_data = sizeof(hfp_task_data) 
                      + num_links * sizeof(hfp_link_data)
                      + num_services * sizeof(hfp_service_data);
        
        /* Allocate and zero our hfp_task_data */
        lHfp = theHfp = PanicUnlessMalloc(size_hfp_data);
        memset(lHfp, 0, size_hfp_data);
        
        /* Set pointers - NB. (lHfp + 1) compiles to (lHfp + (1 * sizeof(hfp_task_data))) */
        lHfp->links    = (hfp_link_data*)(lHfp + 1);
        lHfp->services = (hfp_service_data*)(lHfp->links + num_links);
        lHfp->top      = (hfp_service_data*)(lHfp->services + num_services);
        
        PRINT(("HFP Task Data taking up %d words\n", size_hfp_data));
        PRINT(("%d Words for main task\n",           sizeof(hfp_task_data)));
        PRINT(("%d Words for links\n",               sizeof(hfp_link_data) * num_links));
        PRINT(("%d Words for services\n",            sizeof(hfp_service_data) * num_services));
    
        /* Set the handler function */
        lHfp->task.handler = hfpProfileHandler;
        
        /* Mask out unsupported features. */
        lHfp->hf_supported_features = (config->supported_features & ~HFP_ENHANCED_CALL_CONTROL);
        
        if(!supportedProfileIsHfp106(profile))
            lHfp->hf_supported_features &= ~HFP_CODEC_NEGOTIATION;
        
        /* Codec negotiation is supported */
        if(hfFeatureEnabled(HFP_CODEC_NEGOTIATION))
            hfpWbsEnable(config->supported_wbs_codecs);
        
        /* Set the number of link loss reconnect attempts */
        lHfp->link_loss_time     = config->link_loss_time;
        lHfp->link_loss_interval = config->link_loss_interval;

        /* Set up other config options */
        lHfp->extra_indicators = extra_indicators;
        lHfp->optional_indicators = config->optional_indicators;
        lHfp->disable_nrec = config->disable_nrec;
        lHfp->extended_errors = config->extended_errors;
        lHfp->csr_features = config->csr_features;

        /* Store the app task so we know where to return responses */
        lHfp->clientTask = theAppTask;

        if(config->supported_profile == hfp_no_profile)
        {
            hfpInitCfmToApp(hfp_init_success);
            return;
        }

        /* Connection related state updated in separate function */
        for_all_links(link)
        {
            /*hfpLinkReset(link, FALSE); - link already memset to 0 above */
            link->ag_supported_features = (AG_THREE_WAY_CALLING | AG_IN_BAND_RING);

            /*  Linkloss should be managd by the HFP lib by default  */
            link->manage_linkloss = TRUE;
        }
    
        /* Ensure only one HFP version is specified (1.6 takes priority) */
        if(supportedProfileIsHfp106(profile))
            profile &= ~hfp_handsfree_profile;
        
        /* Set up services and begin registration */
        hfpServicesInit(profile, config->multipoint);
        hfpServiceChannelRegister(theHfp->services);
        
        /* We want sync connect notifications */
        ConnectionSyncRegister(&theHfp->task);
    }
}


/****************************************************************************
NAME
    hfpInitRfcommRegisterCfm

DESCRIPTION
    Rfcomm channel has been allocated.

RETURNS
    void
*/
void hfpInitRfcommRegisterCfm(const CL_RFCOMM_REGISTER_CFM_T *cfm)
{
    if(cfm->status == success)
    {
        static u8 index = 0;
        hfp_service_data* service = &theHfp->services[index];
        
        /* We may not get back the channel we requested, update
           the service entry before registering an SDP record */
        service->rfc_server_channel = cfm->server_channel;
        hfpRegisterServiceRecord(service);
        
        /* Register RFCOMM channel for the next service */
        if(service < HFP_SERVICE_TOP)
        {
            service = &theHfp->services[++index];
            hfpServiceChannelRegister(service);
        }
        
        return;
    }
    
    /* Request failed or no free service */
    hfpInitCfmToApp(hfp_init_rfc_chan_fail);
}


/****************************************************************************
NAME
    hfpInitSdpRegisterComplete

DESCRIPTION
    SDP registration has completed

RETURNS
    void
*/
void hfpInitSdpRegisterComplete(hfp_lib_status status)
{
    if(status == hfp_success)
    {
        /* Get the service we requested to register */
        hfp_service_data* service = hfpGetServiceFromChannel(theHfp->busy_channel);
        if(service)
        {
            if(service == HFP_SERVICE_TOP)
            {
                /* Last service successfully registered, done. */
                hfpInitCfmToApp(hfp_init_success);
                theHfp->initialised = TRUE;
            }
            /* Make sure we clear the registering channel */
            theHfp->busy_channel = 0;
            return;
        }
    }
    /* Either request failed or couldn't find the right service */
    hfpInitCfmToApp(hfp_init_sdp_reg_fail);
}


/****************************************************************************
NAME
    hfpInitSdpRegisterCfm

DESCRIPTION
    SDP register request from Connection library has returned

RETURNS
    void
*/
void hfpInitSdpRegisterCfm(const CL_SDP_REGISTER_CFM_T *cfm)
{
    hfp_lib_status status = (cfm->status == success) ? hfp_success : hfp_fail;
    if(status == hfp_success)
    {
        /* Get the service we requested to register */
        hfp_service_data* service = hfpGetServiceFromChannel(theHfp->busy_channel);
        /* Store the service handle */
        service->sdp_record_handle = cfm->service_handle;
    }
    hfpInitSdpRegisterComplete(status);
}
