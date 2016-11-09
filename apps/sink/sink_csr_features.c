/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_csr_features.c

DESCRIPTION
    handles the csr to csr features

NOTES

*/

#include "sink_debug.h"
#include "sink_private.h"
#include "sink_events.h"
#include "sink_csr_features.h"

#include <hfp.h>

#ifdef DEBUG_CSR2CSR
#else
#endif

void csr2csrHandleTxtInd (void )
{
    LOGD("CSR2CSR TXT IND\n");
}

void csr2csrHandleSmsInd (void )
{
    LOGD("CSR2CSR SMS IND\n");
}

void csr2csrHandleSmsCfm(void )
{
    LOGD("CSR2CSR SMS CFM\n");
}

void csr2csrHandleAgBatteryRequestInd ( void )
{
    LOGD("CSR2CSR BATTERY REQUEST IND\n");
	MessageSend (&theSink.task , EventUsrBatteryLevelRequest , 0 );
}

void csr2csrHandleAgBatteryRequestRes(u8 idx)
{
    const unsigned char csr2csr_battery_level_indications[4] = {2,4,7,9};

    LOGD("CSR2CSR BATTERY REQUEST RES %d", idx);
    if(idx < sizeof(csr2csr_battery_level_indications))
    {
        u16 batt_level = csr2csr_battery_level_indications[idx];
        LOGD(" [%d]", batt_level);
        /* Attempt to send indication to both AGs (HFP will block if unsupported) */
        HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, batt_level);
        HfpCsrFeaturesBatteryLevelRequest(hfp_secondary_link, batt_level);
    }
    LOGD("\n");
}
