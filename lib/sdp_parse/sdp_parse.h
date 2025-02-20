/****************************************************************************
Copyright (c) 2006 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    sdp_parse.h
    
DESCRIPTION
	Header file for the SDP Parse library, allows access to / insertion of 
	service record attributes. NB. Insertion will fail if no memory has been 
	allocated for the attribute you are attempting to insert in the original 
	service record.  
*/

/*!
@file	sdp_parse.h
@brief	Header file for the SDP Parse library 
		Allows access to / insertion of service record attributes. NB. Insertion 
		will fail if no memory has been allocated for the attribute you are 
		attempting to insert in the original service record.  
*/

#ifndef SDP_H_
#define SDP_H_

#include <service.h>

/************************************ Functions *****************************/

/*!
	@brief Access the profile version from the Service Record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param service_class Service Class to attempt to match
	
	@param profile Filled in with profile version on success, filled with zero otherwise
	
    Returns TRUE successful in accessing the profile version, FALSE otherwise
*/

bool SdpParseGetProfileVersion(const u8 size_service_record, const u8* service_record, u16 service_class, u16* profile);

/*!
	@brief Insert a profile version into the Service Record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param service_class Service Class to attempt to match
	
	@param profile Profile version to insert into the Service Record
	
    Returns TRUE successful in inserting the profile version, FALSE otherwise
*/

bool SdpParseInsertProfileVersion(const u8 size_service_record, const u8* service_record, u16 service_class, u16 profile);

/*!
	@brief Access the Supported Features from the Service Record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param features Filled in with supported features on success, filled with zero otherwise

    Returns TRUE successful in  accessing the supported features, FALSE otherwise
*/

bool SdpParseGetSupportedFeatures(const u8 size_service_record, const u8* service_record, u16* features);

/*!
	@brief insert Supported Features into the Service Record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param features Supported features to insert into the Service Record

    Returns TRUE successful in inserting the supported features, FALSE otherwise
*/

bool SdpParseInsertSupportedFeatures(const u8 size_service_record, const u8* service_record, u16 features);

/*!
	@brief This function allows access to all RFCOMM server channels in the service record, a pointer to an allocated
		   u8 array should be passed in along with it's size.  This array will be filled with the channel numbers
		   found and the number of channels will be returned via chans_found

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param size_chans The size of the array passed in
	
	@param chans Pointer to an allocated u8 array to be filled with the RFCOMM channels found on success, unaltered 
		   otherwise
		   
	@param chans_found The number of channels found

    Returns TRUE successful in finding one or more Server Channels, FALSE otherwise
*/

bool SdpParseGetMultipleRfcommServerChannels(u8 size_service_record, u8* service_record, u8 size_chans, u8** chans, u8* chans_found);

/*!
	@brief Insert the RFCOMM Server Channel into the Service Record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param chan The RFCOMM channel number to insert into the service record

    Returns TRUE successful in inserting the Server Channel, FALSE otherwise
*/

bool SdpParseInsertRfcommServerChannel(const u8 size_service_record, const u8* service_record, u8 chan);

/*!
	@brief Access a user specified arbitrary attribute from the Service Record, this will only work on simple 
		   attributes and not list entries such as profile version. 

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param id The Attribute ID to search for
	
	@param val Filled in with the value of the attribute found on success, filled with zero otherwise

    Returns TRUE successful in accessing the desired attribute, FALSE otherwise
*/

bool SdpParseGetArbitrary(const u8 size_service_record, const u8* service_record, ServiceAttributeId id, u32* val);


/*!
	@brief Insert a user specified arbitrary attribute into the Service Record, this will only work on simple 
		   attributes and not list entries such as profile version. 

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param id The attribute ID at which to insert the value
	
	@param val The value to be inserted into the service record

    Returns TRUE successful in inserting the desired attribute into the service record, FALSE otherwise
*/

bool SdpParseInsertArbitrary(const u8 size_service_record, const u8* service_record, ServiceAttributeId id, u32 val);


/*! 
	@brief Access the service name attribute in the service record, a pointer to an allocated char array should be 
		   passed in, this will be filled with as much of the service name as it can hold.

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search

	@param size_service_name The size of the array being passed in (remember to leave room for a NULL char)
	
	@param service_name Pointer to an allocated char array which will be filled with the service name on success 
		   (NULL terminated) and unaltered otherwise
	
	@param length_service_name The number of chars of the array used (including the NULL char)
	
    Returns TRUE if successful in accessing the service name, FALSE otherwise
*/

bool SdpParseGetServiceName(const u8 size_service_record, const u8* service_record, u8 size_service_name, char** service_name, u8* length_service_name);

/*! 
	@brief Insert a user specified service name into the service record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param service_name The service name to insert. NB This must be the same length or shorter than the length of the service
		   name at initialisation and a maximum of 32 characters, or this will fail.  If the new service name is shorter than 
		   the original the remaining space will be filled with ' '.

    Returns TRUE if successful in inserting the service name into the service record, FALSE otherwise
*/

bool SdpParseInsertServiceName(const u8 size_service_record, const u8* service_record, char* service_name);

/*! 
	@brief Access the PBAP Repository attribute in the service record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param repos Filled in with the value of the PBAP Repository on success, filled with zero otherwise

    Returns TRUE if successful in accessing the PBAP Repository, FALSE otherwise
*/

bool SdpParseGetPbapRepos(const u8 size_service_record, const u8* service_record, u8* repos);

/*! 
	@brief Insert PBAP Repository value

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param repos The service value to insert.

    Returns TRUE if successful in inserting the PBAP Repository value, FALSE otherwise
*/

bool SdpParseInsertPbapRepos(const u8 size_service_record, const u8* service_record, u8 repos);

/*! 
	@brief Access the MASInstanceID attribute in the service record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param repos Filled in with the value of the MASInstanceID on success,
                 filled with zero otherwise

    Returns TRUE if successful in accessing the MASInstanceID, FALSE otherwise
*/

bool SdpParseGetMapMasInstance( const u8 size_service_record, 
                                u8* service_record, 
                                u8* instance_id);

/*! 
	@brief Access the SupportedMessageTypes attribute in the service record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param repos Filled in with the value of the SupportedMessageTypes on success,
                 filled with zero otherwise

    Returns TRUE if successful, FALSE otherwise
*/
bool SdpParseGetMapMasMsgFeature( const u8 size_service_record, 
                                  u8* service_record, 
                                  u8* msg_feature);


/*!
	@brief Access the GoepL2CapPsm attribute from the Service Record

	@param size_service_record Size of the service record

	@param service_record Pointer to the Service Record to search
	
	@param psm Filled in with GoepL2CapPsm attribute value.

    Returns TRUE successful in accessing the GoepL2CapPsm attribute Id,
    FALSE otherwise
*/

bool SdpParseGetGoepL2CapPsm( const u8 size_service_record, 
                              const u8* service_record,
                              u16* psm );
		
#endif /* SDP_H_ */
