/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    a2dp_caps_parse.h
    
DESCRIPTION
	
*/

#ifndef A2DP_CAPS_PARSE_H_
#define A2DP_CAPS_PARSE_H_


#include "a2dp.h"


/****************************************************************************
NAME	
	a2dpConvertUint8ValuesToUint32

DESCRIPTION
	Make a u32 out of consecutive array of u8 values.

*/
u32 a2dpConvertUint8ValuesToUint32(const u8 *ptr);


/****************************************************************************
NAME	
	a2dpConvertUint8ValuesToUint16

DESCRIPTION
	Make a u16 out of consecutive array of u8 values.

*/
u16 a2dpConvertUint8ValuesToUint16(const u8 *ptr);


/****************************************************************************
NAME	
	a2dpFindStdEmbeddedCodecCaps

DESCRIPTION
	Locates the start of the specified standard codec specific information, which may be embedded 
    within the supplied codec caps.

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool a2dpFindStdEmbeddedCodecCaps(const u8 **codec_caps, u8 embedded_codec);


/****************************************************************************
NAME	
	a2dpFindNonStdEmbeddedCodecCaps

DESCRIPTION
	Locates the start of the specified non-standard codec specific information, which may be embedded 
    within the supplied codec caps.

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool a2dpFindNonStdEmbeddedCodecCaps(const u8 **codec_caps, u32 embedded_vendor, u16 embedded_codec);

/****************************************************************************
NAME	
	a2dpFindMatchingCodecSpecificInformation

DESCRIPTION
	Returns pointer to start of codec specific information if
	the local and remote codecs are compatible.

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
const u8* a2dpFindMatchingCodecSpecificInformation(const u8 *local_caps, const u8 *remote_caps, bool initiating);


/****************************************************************************
NAME	
	a2dpValidateServiceCaps

DESCRIPTION
	Attempts to validate that a support Service Capabilities list can
	be parsed and contains reasonable values.

	This function should allow all valid values, even if the local hardware/software
	does not support them.

	It is also used to validate the caps returned by the remote device, so should not
	be dependent on local settings.

	The 'reconfigure' flag is used to adjust the validation rules depending on if
	the Capabilities supplied are complete, or part of a reconfiguration.

	When 'only_check_structure' is TRUE this function only tests that the structure
	is correct, it does not verify if mandatory entries are present.
    
    When 'ignore_bad_serv_category' is TRUE this function does not return an error for
    service categories that are out of spec.

	When the function returns FALSE, it will write the service category with the error
	and the AVDTP error code to error_category and error_code parameters.

*/
bool a2dpValidateServiceCaps(const u8 *caps, u16 caps_size, bool reconfigure, bool only_check_structure, bool ignore_bad_serv_category,
							  u8 *error_category, u8 *error_code);


/****************************************************************************
NAME	
	a2dpFindCodecSpecificInformation

DESCRIPTION
	Finds the next codec block in a list of caps.
	Passed pointer and size are updated to point to the search result.
	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool a2dpFindCodecSpecificInformation(const u8 **caps, u16 *caps_size);


/****************************************************************************
NAME	
	a2dpAreServicesCategoriesCompatible

DESCRIPTION
    Checks the Services requested in a SET_CONFIG or RECONFIGURE command
    are supported by the local SEP.  It only checks for the Service entry
    and DOES NOT validate the actual service capabilities - that should
    be done by other functions e.g. gavdpFindMatchingCodecSpecificInformation

	IMPORTANT: It assumes the basic structure of the caps is valid. Call
	gavdpValidateServiceCaps() first to make sure.

*/
bool a2dpAreServicesCategoriesCompatible(const u8 *local_caps, u16 local_caps_size,
                                        const u8 *config_caps, u16 config_caps_size,
                                        u8 *unsupported_service);


/****************************************************************************
NAME	
	a2dpGetContentProtection

DESCRIPTION
	Call this function to determine if Content Protection has been selected by searching
	through the configured service capabilities.
	
	
*/
a2dp_content_protection a2dpGetContentProtection(const u8 *ptr, const u16 size_ptr, u8 **returned_caps);


/****************************************************************************
NAME	
	a2dpIsServiceSupported

DESCRIPTION
	Call this function to determine if the specified service has been selected by searching
	through the configured service capabilities.
	
	
*/
bool a2dpIsServiceSupported (u8 service, const u8 *caps, const u16 size_caps);


#endif /* A2DP_CAPS_PARSE_H_ */
