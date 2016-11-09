/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sdp_parse_l2cap.c
    
DESCRIPTION
	Contains functions for accessing the L2CAP related attributes
*/

#include "sdp_parse.h"
#include <service.h>
#include <region.h>

/************************************ Private *****************************/
static bool findGoepL2CapPsm( const u8 length, 
                              const u8* begin, 
                              Region* value )
{
	ServiceDataType type;
	Region record;
	record.begin = begin;
    record.end   = begin + length;

	if (ServiceFindAttribute(&record, saGoepL2CapPsm, &type, value))
		if(type == sdtUnsignedInteger)
		{
			/* Found the Supported Features */
			return TRUE;
		}
	/* Failed */
	return FALSE;
}


/************************************ Public ******************************/
/* Get GoepL2CapPsm */
bool SdpParseGetGoepL2CapPsm( const u8 size_service_record, 
                              const u8* service_record,
                              u16* psm )
{
	Region value;
    if(findGoepL2CapPsm(size_service_record, service_record, &value))
    {
		*psm = (u16) RegionReadUnsigned(&value);
		/* Accessed Successfully */
		return TRUE;
    }
	/* Failed */
    return FALSE;
}

