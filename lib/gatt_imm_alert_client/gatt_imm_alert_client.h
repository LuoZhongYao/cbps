/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

/*!
@file    gatt_imm_alert_client.h
@brief   Header file for the GATT Immediate Alert Client library.

        This file provides documentation for the GATT Immediate Alert Client library
        API (library name: gatt_imm_alert_client).
*/

#ifndef GATT_IMM_ALERT_CLIENT_H
#define GATT_IMM_ALERT_CLIENT_H

#include <csrtypes.h>
#include <message.h>

#include "library.h"


/* This structure is made public to application as application is responsible for managing resources 
* for the elements of this structure. The data elements are indented to use by Immediate Alert Service Client lib only. 
* Application SHOULD NOT access (read/write) any elements of this library structure at any point of time and doing so  
* may cause undesired behaviour of this library functionalities
*/
typedef struct _gatt_imm_alert_client_t
{
    TaskData lib_task;
    Task app_task;
    u16 cid;
    u16 alert_handle;
}_gatt_imm_alert_client_t;

/*! @brief GATT Immediate Alert Service Client[GIASC]Library Instance.
 */
typedef struct _gatt_imm_alert_client_t GIASC_T;


/*! @brief Enumeration of messages a client task may receive from the immediate alert  client library.
 */
typedef enum
{
    GATT_IMM_ALERT_CLIENT_INIT_CFM = GATT_IMM_ALERT_CLIENT_MESSAGE_BASE,  /* Confirmation for Init */
    GATT_IMM_ALERT_CLIENT_SET_ALERT_CFM,    /* Confirmation for Alert level Settings */

    /* Library message limit */
    GATT_IMM_ALERT_CLIENT_MESSAGE_TOP       /* Top of message enumeration */
} GattImmAlertServiceMessageId;

/*!
    @brief Enumeration for Alert level Value of Alert Level characteristic for immediate alert service 
*/

/* For Alert Level characteristic value, refer http://developer.bluetooth.org/
 * gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.
 * characteristic.alert_level.xml
 */
 
typedef enum {

    gatt_imm_alert_level_no          = 0,     /* No Alert request */
    gatt_imm_alert_level_mild        = 1,    /* Mild Alret request */
    gatt_imm_alert_level_high        = 2,    /* High Alert request  */
    gatt_imm_alert_level_reserved
}gatt_imm_alert_set_level;

/*!
    @brief Enumeration for Immediate alert client status code  
*/

typedef enum
{
    gatt_imm_alert_client_status_success,             /* Request was success */
    gatt_imm_alert_client_status_invalid_param,    /* Request failed because of invalid parameters */
    gatt_imm_alert_client_status_not_allowed,       /* Request is not allowed at the moment,something went wrong internally  */
    gatt_imm_alert_client_status_initiated,             /* Request has been initiated, this status follows an indication/confirmation message */
    gatt_imm_alert_client_status_no_connection,   /* There is no GATT connection exists for given CID so that service library can issue a request to remote device */
    gatt_imm_alert_client_status_failed                 /* Request has been failed */
}gatt_imm_alert_client_status;

/*!
    @brief Parameters used by the Initialisation API, valid value of these  parameters are must for library initialisation  
*/
typedef struct
{
     u16 cid;        /*!Connection ID of the GATT connection on which the server side immediate alert service need to be accessed*/
     u16 start_handle;       /*! The first handle of Immediate Alert service need to be accessed*/
     u16 end_handle;         /*!The last handle of Immediate Alert service need to be accessed */
} GATT_IMM_ALERT_CLIENT_INIT_PARAMS_T;

/*!
    @brief Initialises the Immediate Alert Client Library.
     Initialize Immediate alert client library handles, It starts finding out the characteristic handles of immediate alert service.
     Once the initialisation has been completed, GATT_IMM_ALERT_CLIENT_INIT_CFM will be received with 
     status as enumerated as gatt_imm_alert_client_status.'gatt_imm_alert_client_status_success' has to 
     be considered initialisation of library is done successfully and all the required charecteristics has been found out

     NOTE:This interface need to be invoked for every new gatt connection when the client wish to use 
     immediate alert client library  

    @param appTask The Task that will receive the messages sent from this immediate alert client  library.
    @param imm_alert_client A valid area of memory that the service library can use.Must be of at least the size of GIASC_T
    @param client_init_params as defined in GATT_IMM_ALERT_CLIENT_INIT_PARAMS_T , it is must all the parameters are valid
                The memory allocated for GATT_IMM_ALERT_CLIENT_INIT_PARAMS_T can be freed once the API returns.
    @return The status result of calling the API defined in gatt_imm_alert_client_status

    NOTE: gatt_imm_alert_client_status_initiated will be received as indication that client library initiation started. Once on completion 
     GATT_IMM_ALERT_CLIENT_INIT_CFM will be received with a status enumerated as in gatt_imm_alert_client_status

*/
gatt_imm_alert_client_status GattImmAlertClientInit(
                                        Task appTask , 
                                        GIASC_T *const imm_alert_client,
                                        const GATT_IMM_ALERT_CLIENT_INIT_PARAMS_T *const client_init_params);


/*!@brief Immediate alert client library initialisation confirmation 
*/
typedef struct PACK_STRUCT __GATT_IMM_ALERT_CLIENT_INIT_CFM
{
    const GIASC_T *imm_alert_client;        /*! Reference structure for the instance */
    u16 cid;                                          /*! Connection Identifier for remote device */
    gatt_imm_alert_client_status status;      /*!status as per gatt_imm_alert_client_status */
} GATT_IMM_ALERT_CLIENT_INIT_CFM_T;


/*!
    @brief This API is used to set the Alert level on Immediate Alert service of server side .
    
    NOTE:GATT_IMM_ALERT_CLIENT_INIT_CFM_T should return gatt_imm_alert_client_status_success before using this API,
    eles interface will fail

    @param imm_alert_client Memory area used for initiating Immediate alert client, used in GattImmAlertClientInit() .
    @param alert_level  Alert level that need to be set at server side, as defined in gatt_imm_alert_set_level
    
    @return The status result of calling the API as defined in gatt_imm_alert_client_status.
    
    NOTE: gatt_imm_alert_client_status_initiated will be returned as a indication that alert level is being processed,
    GATT_IMM_ALERT_CLIENT_SET_ALERT_CFM will be reived as a confirmation that the request has been sent to
    server, but this does not guarantee that the write request is reached server; as set alert level does not expect 
    any confirmation message from server.

*/
gatt_imm_alert_client_status GattImmAlertClientSetAlertLevel(
                                        const GIASC_T *const imm_alert_client,
                                        const gatt_imm_alert_set_level alert_level);

/*!@brief Immediate alert client library initialisation confirmation 
  NOTE: This confirmation indicates the message has been successfully send to server, not guarantee  that 
  server is received the same or acted on the alert level that has been sent.
*/
typedef struct PACK_STRUCT __GATT_IMM_ALERT_CLIENT_SET_ALERT_CFM
{
    const GIASC_T *imm_alert_client;        /*! Reference structure for the instance */
    u16 cid;                                                                     /*! Connection Identifier for remote device */
    gatt_imm_alert_client_status status;                                  /*!status as per gatt_imm_alert_client_status */
} GATT_IMM_ALERT_CLIENT_SET_ALERT_CFM_T;

/*!
    @brief When a GATT connection is removed, the application must remove all client service instances that were
    associated with the connection (using the CID value).
    This is the clean up routine as a result of calling the GattImmAlertClientInit API. That is,
    the GattImmAlertClientInit API is called when a connection is made, and the GattImmAlertClientDestroy is called 
    when the connection is removed.

    @param imm_alert_client The client instance that was passed into the GattImmAlertClientInit API.
    @param cid The connection ID.

    
    @return The status result of calling the API.

*/
gatt_imm_alert_client_status GattImmAlertClientDestroy(GIASC_T *imm_alert_client, u16 cid);

#endif /* GATT_IMM_ALERT_CLIENT_H */

