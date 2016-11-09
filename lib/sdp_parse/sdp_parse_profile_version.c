/****************************************************************************
Copyright (c) 2006 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sdp_parse_profile_version.c
    
DESCRIPTION
	Contains functions for accessing the profile version field in a service 
	record
*/

#include "sdp_parse.h"
#include <service.h>
#include <region.h>

/************************************ Private *****************************/

/* Find Profile Version */

static bool findProfileVersion(const u8 size_service_record, const u8* service_record, const u16 service_class, Region* value)
{	
	ServiceDataType type;
    Region record, protocols, protocol;
    record.begin = service_record;
    record.end   = service_record + size_service_record;
	
	/* Move protocols to Profile Descriptor List */
    while(ServiceFindAttribute(&record, saBluetoothProfileDescriptorList, &type, &protocols))
	{
		if(type == sdtSequence)
		{
			/* Move protocol through pdl sections */
	    	while(ServiceGetValue(&protocols, &type, &protocol))
			{
				/* Move to first section element (if no element - move to next section) */
				if(ServiceGetValue(&protocol, &type, value))
				{
					/* Move value through section elements until service class found */
					while(!RegionMatchesUUID32(value, (u32)service_class))
					{
						/* If no more values to read in - failed */
						if(!ServiceGetValue(&protocol, &type, value))
							return FALSE;
					}
				
					/* Assume profile version follows service class, so read + return */
					if(ServiceGetValue(&protocol, &type, value) && type == sdtUnsignedInteger)
		       	 		return TRUE;
				}
			}
		}
	}
    /* Failed */
    return FALSE; 
}

/************************************ Public ******************************/

/* Access Profile Version */

bool SdpParseGetProfileVersion(const u8 size_service_record, const u8* service_record, u16 service_class, u16* profile)
{
	Region value;
    if (findProfileVersion(size_service_record, service_record, service_class, &value))
    {
        *profile = (u16) RegionReadUnsigned(&value);
		/* Accessed Successfully */
        return TRUE;
    }
	/* Failed */
    return FALSE;
}

/* Insert Profile Version */

bool SdpParseInsertProfileVersion(const u8 size_service_record, const u8* service_record, u16 service_class, u16 profile)
{
	Region value;
    if (findProfileVersion(size_service_record, service_record, service_class, &value) && RegionSize(&value) == 2)
    {
            RegionWriteUnsigned(&value, (u32) profile);
			/* Inserted Successfully */
            return TRUE;
    }
	/* Failed */
    return FALSE;
}
