/****************************************************************************
Copyright (c) 2006 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sdp_parse_features.c
    
DESCRIPTION
	Contains functions for accessing the supported features field in a service 
	record
*/

#include "sdp_parse.h"
#include <service.h>
#include <region.h>

/************************************ Private *****************************/

/* Find Supported Features */

static bool findSupportedFeatures(const u8 length, const u8* begin, Region* value)
{
	ServiceDataType type;
	Region record;
	record.begin = begin;
    record.end   = begin + length;

	if (ServiceFindAttribute(&record, saSupportedFeatures, &type, value))
		if(type == sdtUnsignedInteger)
		{
			/* Found the Supported Features */
			return TRUE;
		}
	/* Failed */
	return FALSE;
}

/************************************ Public ******************************/

/* Access Supported Features */

bool SdpParseGetSupportedFeatures(const u8 size_service_record, const u8* service_record, u16* features)
{
	Region value;
    if(findSupportedFeatures(size_service_record, service_record, &value))
    {
		*features = (u16) RegionReadUnsigned(&value);
		/* Accessed Successfully */
		return TRUE;
    }
	/* Failed */
    return FALSE;
}

/* Insert Supported Features */

bool SdpParseInsertSupportedFeatures(const u8 size_service_record, const u8* service_record, u16 features)
{
	Region value;

	if (findSupportedFeatures(size_service_record, service_record, &value) && RegionSize(&value) == 2)
	{
		RegionWriteUnsigned(&value, (u32) features);
		/* Inserted Successfully */
		return TRUE;
	}
	/* Failed */
	return FALSE;
}
