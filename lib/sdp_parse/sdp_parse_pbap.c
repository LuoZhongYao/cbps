/****************************************************************************
Copyright (c) 2006 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sdp_parse_arbitrary.c
    
DESCRIPTION
	Contains functions for accessing PBAP specific fields in a service record
*/

#include "sdp_parse.h"
#include <service.h>
#include <region.h>

/************************************ Private *****************************/

/* Find PBAP Repository */

static bool findPbapRepos(const u8 size_service_record, const u8* service_record, Region* value)
{
	ServiceDataType type;
    Region record;
    record.begin = service_record;
    record.end   = service_record + size_service_record;
	
	if (ServiceFindAttribute(&record, saPbapRepos, &type, value))
		if(type == sdtUnsignedInteger)
		{
			/* Found the Attribute Field */
			return TRUE;
		}
	/* Failed */
	return FALSE;
}

/************************************ Public ******************************/

/* Access PBAP Repository */

bool SdpParseGetPbapRepos(const u8 size_service_record, const u8* service_record, u8* repos)
{
	Region value;
	if(findPbapRepos(size_service_record, service_record, &value))
	{
		*repos = RegionReadUnsigned(&value);
		/* Accessed Successfully */
		return TRUE;
	}
	/* Failed */
	return FALSE;
}

/* Insert PBAP Repository */

bool SdpParseInsertPbapRepos(const u8 size_service_record, const u8* service_record, u8 repos)
{
	Region value;
	if(findPbapRepos(size_service_record, service_record, &value))
	{
		RegionWriteUnsigned(&value, repos);
		/* Inserted Successfully */
		return TRUE;
	}
	/* Failed */
	return FALSE;
}
