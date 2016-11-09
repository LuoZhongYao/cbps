/* Copyright (c) 2014 - 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

/*!
@file    gatt_hid_client.h
@brief   Header file for the GATT HID Client library.

        This file provides documentation for GATT HID Client library
        API (library name: gatt_hid_client).
*/


#ifndef GATT_HID_CLIENT_H
#define GATT_HID_CLIENT_H

#include <csrtypes.h>
#include <message.h>

#include <library.h>

#include "gatt_manager.h"

/*! @brief GATT HID Client Configuration defines.
 * Either all the configuration parameters to be set to valid values or set to recommended configuration values as below.
 * If GATT_HID_CLIENT_CONFIG_PARAMS_T is set to NULL,  all parameters are configured as default values as below 
 * and boot mode will be supported
 */

/*! Maximum Allowed Boot Mode Client Characteristic Configuration per instance.
* In case of Combo device, it may be 2 input reports in Boot mode.
*/
#define MAX_NUM_BOOT_CCD 2

/*! Maximum Allowed Report Mode Client Characteristic Configuration per instance
*/
#define MAX_NUM_REPORT_CCD 5

/*! Maximum characteristics to be stored while discovering the characteristic UUID's per instance
*/
#define MAX_CHAR_UUID_HANDLE 15

/*!  Maximum number of report could be supported in the remote device per instance.
 * Including the Input, Output and Feature reports.
 *  5 input reports and 1 report for Output (normally with Keyboard).1 report for Feature
 */
#define MAX_NUM_HID_REPORTS                                (7)

/*! @brief Report ID mapping structure for HID reports 
 */
typedef struct
{
    u16 report_handle;           /*! Handle of LE HID Report Reference characteristic at remote device */
    u16 report_id;               /*! Report id  in the LE HID Report Reference characteristic */
    u16 type;                    /*! Report type in the LE HID Report Reference characteristic */
    u16 report_reference_handle; /*! Report handle of the LE HID Report characteristic  */
    bool notify;                    /*! Flag set when notifications should be recieved from this report */
}gatt_hid_client_report_id_map_t;

/*!Enumeration for ccd handle types 
 */
typedef enum
{
    gatt_hid_boot_ccd =0, /* Boot Mode CCD */
    gatt_hid_report_ccd    /* Report Mode CCD */
}gatt_hid_ccd_type;

/*!Structure type used to allocate memory internal to HID client Lib to manage HID handles,
* This structure is configured as per GATT_HID_CLIENT_CONFIG_PARAMS_T in GattHidClientInit() interface.
* Application is not supposed to manipulate this structure at any time,if then unpredictable behaviour of
* HID client lib may happen
*/
typedef struct 
{
    u16 ccd; /*! Stores the client characteristic configuration descriptor handle */
    u16 ccd_type; /*! Stores the client characteristic configuration decriptor type */
    u16 characterisitc_handle; /*! Stores the characterisitc report handle for which the ccd is available */
}gatt_hid_ccd_handle_mem_t;

/* Structure to hold HID boot mode handles */
typedef struct
{
    u16 boot_kb_input_report_handle; /*! Boot mode keyboard input report */
    u16 boot_mouse_input_report_handle; /*! Boot mode keyboard output report */
    u16 boot_output_report_handle; /* Boot mode output report */
}gatt_hid_boot_mode_handles_t;

/* This structure is made public to application as application is responsible for managing resources 
* for the elements of this structure. The data elements are indented to use by HID Client lib only. 
* Application SHOULD NOT access (read/write) any elements of this library structure at any point of time and doing so  
* may cause undesired behaviour of this library functionalities
*/
typedef struct _gatt_hid_client_t
{
    TaskData lib_task; /*! Lib Task*/
    Task app_task; /*!Application Registered Task */
    u16 protocol_handle; /*! Discovered Protocol handle */
    u16 info_handle; /*! Discovered HID info handle */
    u16 control_handle; /*! Discovered HID control handle */
    u16 external_report_reference_handle; /*! Discovered HID external reference handle */
    u16 report_map_handle; /*! Discovered report map handle */
    u16 pending_request; /*! Any read/write pending? */
    gatt_hid_ccd_handle_mem_t *ccd_handles; /*! This memory is managed by lib , allocation will happen in Init and Free will happen in Deinit */
    gatt_hid_boot_mode_handles_t boot_handles; /*! Handles for boot mode */
    gatt_hid_client_report_id_map_t *report_id_map; /*! Stores HID report-id report-handle mapping */
    unsigned num_report_id:5; /*! Num of report id's available with HID service */
    unsigned num_boot_ccd:5; /*! Num of boot ccd available with HID service */
    unsigned num_report_ccd:5; /*! Num report ccd available with HID service */
    unsigned discovery_in_progress:1; /*! Discovery is in progress for this HID instance ? */
    unsigned num_ccd_cfm:5; /*! Num ccd registration happened */
    unsigned boot_mode_supported:1; /*! Is boot mode is supported in this instance */
    unsigned notifitcation_enabled:1; /*! Notification has been enabled/disabled */
    unsigned _SPARE1_:9; /*! Spare bits available */
}_gatt_hid_client_t;


/*! @brief GATT HID Client [GHIDC]Library Instance.
 */
typedef struct _gatt_hid_client_t GHIDC_T;

/*! @brief Enumeration of messages a client task may receive from the hid client library.
 */
typedef enum
{
    GATT_HID_CLIENT_INIT_CFM = GATT_HID_CLIENT_MESSAGE_BASE,/*! 00 */
    GATT_HID_CLIENT_GET_PROTOCOL_CFM, /*! 01 */
    GATT_HID_CLIENT_READ_INFO_CFM, /*!02 */
    GATT_HID_CLIENT_READ_EXT_REF_CFM, /*! 03 */
    GATT_HID_CLIENT_REPORT_ID_MAP_CFM, /*! 04 */
    GATT_HID_CLIENT_GET_REPORT_CFM, /*! 05 */
    GATT_HID_CLIENT_READ_REPORT_MAP_CFM, /*! 06 */
    GATT_HID_CLIENT_READ_BOOT_REPORT_CFM, /*! 07 */
    GATT_HID_CLIENT_SET_PROTOCOL_CFM, /*! 08 */
    GATT_HID_CLIENT_CONTROL_REQ_CFM, /*! 09 */
    GATT_HID_CLIENT_NOTIFICATION_REG_CFM, /*! 10*/
    GATT_HID_CLIENT_NOTIFICATION_REPROTID_REG_CFM, /*! 11 */
    GATT_HID_CLIENT_SET_REPORT_CFM, /*! 12 */
    GATT_HID_CLIENT_WRITE_BOOT_REPORT_CFM, /*!13 */
    GATT_HID_CLIENT_NOTIFICATION_IND, /*! 14 */
    GATT_HID_CLIENT_READ_CCD_CFM, /*! 15 */
    /* Library message limit */
    GATT_HID_CLIENT_MESSAGE_TOP /*! 16 */
} gatt_hid_client_message_id_t;

/*!
    @brief Enumeration for HID client status code  
*/
typedef enum
{
    gatt_hid_client_status_success, /* Request was success */
    gatt_hid_client_status_invalid_param, /* Request failed because of invalid parameters */
    gatt_hid_client_status_not_allowed, /* Request is not allowed at the moment,Try after some time */
    gatt_hid_client_status_initiated, /* Request has been initiated, this status follows an indication/confirmation message */
    gatt_hid_client_status_no_connection, /* There is no GATT connection exists for given CID so that service library can issue a request to remote device */
    gatt_hid_client_status_registration_failed, /* Registration  with GATT manager failed */
    gatt_hid_client_status_failed, /* Request has been failed */
    gatt_hid_client_status_not_supported, /* Request is not supported for current instance */
    gatt_hid_client_status_insufficient_authentication, /*Authentication is insufficient for the operation */
    gatt_hid_client_status_insufficient_memory /* Not Enough memory available */
}gatt_hid_client_status;

/*!
    @brief HID notifitcaion indication, This indication will be received on each notification from HID service   
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_NOTIFICATION_IND
{
    const GHIDC_T *hid_client;/*! Reference structure for the instance */
    u16 cid; /*! Connection ID for the instance */
    u16 report_id; /*! Report ID */
    u16 size_value; /*! Size of value element */
    u8 value[1]; /*! Value in notification */
} GATT_HID_CLIENT_NOTIFICATION_IND_T;

/*!
    @brief Parameters used by the Initialisation API, valid value of these  parameters are must for library initialisation  
*/
typedef struct
{
     u16 cid; /*! Connection ID of the GATT connection on which the server side HID service need to be accessed*/
     u16 start_handle; /*! The first handle of HID service need to be accessed*/
     u16 end_handle; /*! The last handle of HID service need to be accessed */
} GATT_HID_CLIENT_INIT_PARAMS_T;

/*!
    @brief Parameters used by the Initialisation API, Used to configure HID lib memory for handles  
    NOTE: Maximum Configurable value of each parameters is 0x1F, ie it holds a 5 bit value.
*/
typedef struct
{
     /*! Maximum Number Of reports supported by this instance of HID Client 
     * This structure element allocate lib memory of (sizeof(u16)*sizeof(gatt_hid_client_report_id_map_t))
     */
     u16 max_num_report;
     /*! Maximum number of report mode client characteristic descriptors supported by this  instance of HID client 
     * This structure element allocate lib memory of  (sizeof(u16)*max_num_reportmode_ccd)
     */
     u16 max_num_reportmode_ccd;
      /*! Maximum number of boot mode client characteristic descriptors supported by this  instance of HID client 
      * This structure element allocate lib memory of  (sizeof(u16)*max_num_bootmode_ccd) 
     */
     u16 max_num_bootmode_ccd;
     /*! Maximum number of characteristic handles supported by this instance of HID client 
     * This structure element allocate lib memory of  (sizeof(u16)*max_num_char_handles)
     */
     u16 max_num_char_handles; 
     /*! To Indicate LIB that this instance of HID client required to support boot mode or not
       If TRUE, then max_num_bootmode_ccd is valid 
     */
     u16 is_boot_mode_supported; 
}GATT_HID_CLIENT_CONFIG_PARAMS_T;


/*!
    @brief Initialises the HID Client Library.
     Initialize HID Library, It starts finding out the characteristic handles of HID service at the remote side
     as per GATT_HID_CLIENT_INIT_PARAMS_T.
     
     NOTE:This interface need to be invoked for every new gatt connection when the client wish to use 
     HID client library  

    @param appTask The Task that will receive the messages sent from this HID client  library.
    @param hid_client A valid area of memory that the service library can use.Must be of at least the size of GHIDC_T
    @param hid_init_params as defined in GATT_HID_CLIENT_INIT_PARAMS_T , all the parameters should be valid
    @param config_params Configure Memory for storing dynamic length handles as refered in GATT_HID_CLIENT_CONFIG_PARAMS_T
    @param discover_handles, if discover_handles=TRUE all handles will be discovered by GATT lib and if it is FALSE, non of the handles are
                 discovered. It is assumed in FALSE case application will be providing a valid hid_client memory which was used in 
                 previous sucessful initialisation
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that client library initiation started.
    Once initiation has been completed GATT_HID_CLIENT_INIT_CFM_T will be recived with status define in gatt_hid_client_status.
    All other HID client Library Interfaces are valid only when This API retuns a success

    NOTE: discover_handles can be used by application in such a way that, if the GATT connection is from same device and
    hid_client instance is not freed, in this case setting discover_handles = FALSE will speed up connection as lib will use
    already discovered handles for further read and write and no more discovery has been performed.
*/

gatt_hid_client_status GattHidClientInit(
                                Task appTask , 
                                GHIDC_T *const hid_client,
                                const GATT_HID_CLIENT_INIT_PARAMS_T *const hid_client_init_params,
                                const GATT_HID_CLIENT_CONFIG_PARAMS_T *const config_params,
                                bool discover_handles);
/*!
    @brief HID library Init confirmation
             This confirmation will be received once HID library init is completed
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_INIT_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 num_report_id; /*! Number of report ID's supported */
}GATT_HID_CLIENT_INIT_CFM_T;


/*!
    @brief De Initialises the HID Client Library.
     DeInitialize HID library handles and Unregister HID client form GATT manager 
     
     NOTE: his API need to be invoked in any time if application is no more going to use the HID Client lib 
     instance which is instantiated using GattHidClientInit().
     This interface need to be invoked for failure of GattHidClientInit() or if GATT_HID_CLIENT_INIT_CFM_T
     is recevied with status other than gatt_hid_client_status_success. 
     For every GattHIDClientInit(), there should be a GattHidClientInit() call, once application no more
     need to use the HID lib. Failure to invoke GattHidClientDeInit() in error case or disconnect case can lead
     to memory leak in HID lib and Application may not able to use HID lib in future.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_success will be received as indication that successful deinit happened.

*/
gatt_hid_client_status GattHidClientDeInit(
                                GHIDC_T *const hid_client);


/*!
    @brief Get the current HID service protocol mode .

     API used to read the protocol mode of remote HID service on which the HID service is instantiated

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that get protocol request has been initiated. 
    On completion GATT_HID_CLIENT_GET_PROTOCOL_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
*/
gatt_hid_client_status GattHidGetProtocol(
                                GHIDC_T *const hid_client);

/*!@brief Enumeration for HID Protocol modes
*/
typedef enum
{
    gatt_hid_protocol_boot = 0, /* boot mode */
    gatt_hid_protocol_report = 1 /* report mode */
} gatt_hid_client_protocol;

/*!
    @brief HID library get protocol confirmation
             This confirmation will be received with protocol of remote HID service 
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_GET_PROTOCOL_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    gatt_hid_client_protocol protocol; /*! Protocol on remote HID service as defined in gatt_hid_client_protocol */
}GATT_HID_CLIENT_GET_PROTOCOL_CFM_T;


/*!
    @brief Set HID protocol  .

     API used to set the remote HID service protocol 

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @param hid_protocol, protocol type as defined in gatt_hid_client_protocol.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that set protocol request has been initiated. 
    On completion  GATT_HID_CLIENT_SET_PROTOCOL_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
    
*/
gatt_hid_client_status GattHidSetProtocol(
                                GHIDC_T *const hid_client,
                                gatt_hid_client_protocol hid_protocol);
/*!
    @brief HID library set protocol confirmation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_SET_PROTOCOL_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
} GATT_HID_CLIENT_SET_PROTOCOL_CFM_T;


/*!
    @brief Read HID information  .

     API used to read the HID informtaion of HID service

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that information request has been initiated. 
    On completion  GATT_HID_CLIENT_GET_PROTOCOL_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
*/
gatt_hid_client_status GattHidReadInformation(
                                GHIDC_T *const hid_client);

/*!
    @brief HID library read information confirmation
             This confirmation will be received with HID informtaion  
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_READ_INFO_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 size_value; /*! Size of data in buffer 'value' */
    u8 value[1]; /*! Data buffer containing HID informtaion */
}GATT_HID_CLIENT_READ_INFO_CFM_T;


/*!
    @brief Read HIDexternal report reference  .

     API used to read the external report reference of HID service

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that external reference request has been initiated. 
    On completion  GATT_HID_CLIENT_READ_EXT_REF_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
*/
gatt_hid_client_status GattHidReadExternalReportReference(
                               GHIDC_T *const hid_client);

/*!
    @brief HID ibrary read ext reference confirmation
             This confirmation will be recived with ext ref UUID  
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_READ_EXT_REF_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 size_value; /*! Size of data in buffer 'value' */
    u8 value[1]; /*! Data buffer containing ext ref UUID  */
}GATT_HID_CLIENT_READ_EXT_REF_CFM_T;

/*!
    @brief Read HID Client characterictic descriptor 

     API used to Read HID Client characterictic descriptor of HID service for specified mode

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @param mode: On which mode CCD need to be read , as defined in gatt_hid_ccd_type

    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that client characteristic descriptor read request 
    has been initiated. On completion  GATT_HID_CLIENT_READ_CCD_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
*/
gatt_hid_client_status GattHidReadCCD(
                               GHIDC_T *const hid_client,                                   
                               gatt_hid_ccd_type mode);

/*!
    @brief HID library client characteristic descriptor read confirmation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_READ_CCD_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 report_id; /*! Report ID */
    u16 size_value; /*! Size of data in buffer 'value' */
    u8 value[1]; /*! Data buffer containing ext ref UUID  */
}GATT_HID_CLIENT_READ_CCD_CFM_T;


/*!
    @brief Read supported report ID's  .

     API used to read report ID and Its Type, this request will fetch the information stored locally in HID lib instance

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that report ID map  has been initiated. 
    On completion  GATT_HID_CLIENT_REPORT_ID_MAP_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
*/
gatt_hid_client_status GattHidGetReportIDMap(
                                GHIDC_T *const hid_client);

/*!
    @brief enumeration for HID report types
*/
typedef enum
{
    gatt_hid_input_report = 0x01,   /*! Input Report */
    gatt_hid_output_report = 0x02, /*! Output Report */
    gatt_hid_feature_report= 0x03   /*! Feature Report */
}gatt_hid_client_report_type;

/*!
    @brief HID ibrary read report ID map confirmation
             This confirmation will be recived with report ID's , The 'value' buffer contains data in format
             value[sizeof(u16)] : report ID
             value[sizeof(u16)+sizeof(u16)]: report type
             This formatting will continue depend in num_reports.
             ie, if only one report available then
             value[0] :report ID
             value[2]: report type as defined in gatt_hid_client_report_type
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_REPORT_ID_MAP_CFM
{
    const GHIDC_T *hid_client;/*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 num_reports; /*! Number of reports available  */
    u8 value[1]; /*! Data buffer containing report ID and report Type  */
}GATT_HID_CLIENT_REPORT_ID_MAP_CFM_T;


/*!
    @brief Get HID report  .

     API used to read report from HID service

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @param report_id, ID for the report. This can be obtained using GattHidGetReportIDMap() interface 
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that Get report request has been initiated. 
    On completion  GATT_HID_CLIENT_GET_REPORT_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
    
*/
gatt_hid_client_status GattHidGetReport(
                               GHIDC_T *const hid_client,
                               u16 report_id);

/*!
    @brief HID library GET report confirmation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_GET_REPORT_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 report_id; /*! Report ID */
    u16 size_value; /*! Size of data in buffer 'value' */
    u8 value[1]; /*! Data Buffer */
}GATT_HID_CLIENT_GET_REPORT_CFM_T;


/*!
    @brief set report input prameters type 
*/
typedef struct
{
    u16 report_id; /*! Report id, this can be obtained from GattHidGetReportIDMap() API */
    u16 report_len; /*! Report length */
    u16 data[1]; /*! Reference data to the report which need to be written*/
}GATT_HID_CLIENT_SET_REPORT_T;

/*!
    @brief set HID report.

     API used to set the HID report

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory area used in  GattHidClientInit() API.
    @param hid_report_data  report parameters as defined in GATT_HID_CLIENT_SET_REPORT_T
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that set report request has been initiated from library. 
    On completion  GATT_HID_CLIENT_SET_REPORT_CFM_T will be received with a status enumerated as in gatt_hid_client_status.
    GattHidSetReport() API can be used ONLY to set reports available in report mode. To write on boot mode specific reports 
    GattHidWriteBootReport() API should be used

*/
gatt_hid_client_status GattHidSetReport(
                                GHIDC_T *const hid_client,                              
                                GATT_HID_CLIENT_SET_REPORT_T *hid_report_data);
/*!
    @brief HID ibrary set report confirmation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_SET_REPORT_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 report_id; /*! Report ID of which confirmation is received */
}GATT_HID_CLIENT_SET_REPORT_CFM_T;


/*!
    @brief Get HID report map  .

     API used to read report map of HID service 

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that read report map request has been initiated. 
    On completion  GATT_HID_CLIENT_READ_REPORT_MAP_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
    
*/
gatt_hid_client_status GattHidReadReportMap(
                               GHIDC_T *const hid_client);

/*!
    @brief HID library read report map confrimation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_READ_REPORT_MAP_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 more_to_come; /*! Mode data to be recived if TRUE,else end of data */
    u16 offset; /*! Offset to the characteristic value in this message */
    u16 size_value; /*! Size of data in buffer 'value' */
    u8 value[1]; /*! Data Buffer */
}GATT_HID_CLIENT_READ_REPORT_MAP_CFM_T;


/*!@brief Enumeration for HID control point characteristics types
*/
typedef enum
{
    gatt_hid_control_suspend = 0x00, /*! Suspend */
    gatt_hid_control_exitsuspend = 0x01 /*! Exit Suspend */
}gatt_hid_client_control_type;

/*!
    @brief Set HID Control Point  .

     API used to set the remote HID control point(supend or exit suspend )

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory used in  GattHidClientInit() API.
    @param ctrl, control type as defined in  gatt_hid_client_control_type
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that set control point request has been initiated. 
    On completion  GATT_HID_CLIENT_CONTROL_REQ_CFM_T will be received with a status
    enumerated as in gatt_hid_client_status
    
*/
gatt_hid_client_status GattHidControlRequest(
                               GHIDC_T *const hid_client,
                               gatt_hid_client_control_type ctrl);

/*!
    @brief HID library set control point confirmation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_CONTROL_REQ_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
}GATT_HID_CLIENT_CONTROL_REQ_CFM_T;


/*!@brief Enumeration for HID notification types
*/
typedef enum
{
   hid_client_boot_mode=0, /*! Boot mode notification */
   hid_client_report_mode /*! Report mode notification */
}gatt_hid_notification_mode;

/*!
    @brief Register for Notifitcaion from HID service.

     API used to Register for notifitcaion to enable/disble notifitcaion from remote device.
     GATT_HID_CLIENT_NOTIFICATION_REG_CFM_T will be recived as a confirmation on this request

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory area used in  GattHidClientInit() API.
    @param enable enabling/disabling notifitcaion  TRUE: Enable , FALSE: Disable
    @param mode: On which mode notification need to be enabled , as defined in gatt_hid_notification_mode
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that notiftcaion request has been initiated from library. 
    On completion  GATT_HID_CLIENT_NOTIFICATION_REG_CFM_T will be received with a status enumerated as in gatt_hid_client_status

*/
gatt_hid_client_status GattHidRegisterForNotification(
                                  GHIDC_T *const hid_client,
                                  bool enable,
                                  gatt_hid_notification_mode mode);

/*!
    @brief HID library notification registration confirmation
             This confirmation will be received on Enabling/Disabling notification 
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_NOTIFICATION_REG_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
} GATT_HID_CLIENT_NOTIFICATION_REG_CFM_T;

/*!
    @brief Register for Notifitcaion from HID service for a particulr report ID .

     API used to Register for notifitcaion to enable/disble notifitcaion from remote device for a particulr input report.
     GATT_HID_CLIENT_NOTIFICATION_REPROTID_REG_CFM_T will be recived as a confirmation on this request

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory area used in  GattHidClientInit() API.
    @param enable enabling/disabling notifitcaion  TRUE: Enable , FALSE: Disable
    @param report_id: Input Report ID on which the notification has to be enabled
    @param mode : boot mode or report mode
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that notiftcaion request has been initiated from library. 
    On completion  GATT_HID_CLIENT_NOTIFICATION_REPROTID_REG_CFM_T will be received with a status enumerated 
    as in gatt_hid_client_status

*/
gatt_hid_client_status GattHidRegisterNotificationForReportID(
                               GHIDC_T *const hid_client,
                               bool enable,
                               u16 report_id,
                               gatt_hid_notification_mode mode);

/*!
    @brief HID library notification registration for report ID confirmation
             This confirmation will be received on Enabling/Disabling notification 
*/
typedef struct
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
} GATT_HID_CLIENT_NOTIFICATION_REPROTID_REG_CFM_T;

/*!@brief Enumeration for HID boot mode report types
*/
typedef enum
{
    hid_client_kb_ouput_report= 0, /*! Key Board Output Report */
    hid_client_kb_input_report, /*! Key Board Input Report */
    hid_client_mouse_input_report /*! Mouse Input Report*/
}gatt_hid_client_boot_mode_report_type;

/*!
    @brief set boot report params 
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_WRITE_BOOT_REPORT
{
  gatt_hid_client_boot_mode_report_type report_type;/*! Report type as defined in gatt_hid_client_boot_mode_report_type */
  u16 report_len; /*! Report length */
  u16 data[1]; /*! Reference data to the report */
}GATT_HID_CLIENT_WRITE_BOOT_REPORT_T;

/*!
    @brief Write HID boot report.

     API used to write to boot report handle

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory area used in  GattHidClientInit() API.
    @param hid_boot_report boot report parameters as defiend in GATT_HID_CLIENT_SET_BOOT_REPORT_T
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that write boot report request has been initiated from library. 
    On completion  GATT_HID_CLIENT_WRITE_BOOT_REPORT_CFM_T will be received with a status enumerated as in gatt_hid_client_status

*/
gatt_hid_client_status GattHidWriteBootReport(GHIDC_T *const hid_client, 
                                GATT_HID_CLIENT_WRITE_BOOT_REPORT_T *hid_boot_report);

/*!
    @brief HID library write boot report confirmation
             This confirmation will be received on Enabling/Disabling notification 
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_WRITE_BOOT_REPORT_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
}GATT_HID_CLIENT_WRITE_BOOT_REPORT_CFM_T;


/*!
    @brief Read HID boot report.

     API used to read to boot report handle

     NOTE:GATT_HID_CLIENT_INIT_CFM_T should return gatt_hid_client_status_success status before invoking this API,
     else the API will call fails.

    @param hid_client A valid area of memory area used in  GattHidClientInit() API.
    @param hid_boot_report boot report parameters as defiend in GATT_HID_CLIENT_SET_BOOT_REPORT_T
    @return The status result of calling the API defined in gatt_hid_client_status

    NOTE: gatt_hid_client_status_initiated will be received as indication that write boot report request has been initiated from library. 
    On completion  GATT_HID_CLIENT_READ_BOOT_REPORT_CFM_T will be received with a status enumerated as in gatt_hid_client_status

*/
gatt_hid_client_status GattHidReadBootReport(GHIDC_T *const hid_client, 
                                gatt_hid_client_boot_mode_report_type report_type);

/*!
    @brief HID library read boot report confirmation
*/
typedef struct PACK_STRUCT __GATT_HID_CLIENT_READ_BOOT_REPORT_CFM
{
    const GHIDC_T *hid_client; /*! Reference structure for the instance */
    gatt_hid_client_status status; /*! status as per gatt_hid_client_status */
    u16 cid; /*! Connection ID */
    u16 more_to_come; /*! Mode data to be recived if TRUE,else end of data */
    u16 offset; /*! Offset to the characteristic value in this message */
    u16 size_value; /*! Size of data in buffer 'value' */
    u8 value[1]; /*! Data Buffer */

}GATT_HID_CLIENT_READ_BOOT_REPORT_CFM_T;

#endif /* GATT_HID_CLIENT_H */

