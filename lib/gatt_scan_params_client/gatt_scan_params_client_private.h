
/* Copyright (c) 2015 Qualcomm Technologies International, Ltd. */
/* Part of ADK 4.0 */

#ifndef GATT_SCAN_PARAMS_CLIENT_PRIVATE_H
#define GATT_SCAN_PARAMS_CLIENT_PRIVATE_H

#include <csrtypes.h>
#include <message.h>
#include <panic.h>

/* Macros for creating messages */
#define MAKE_SCAN_PARAMS_CLIENT_MESSAGE(TYPE) MESSAGE_MAKE(message,TYPE##_T)

/* Defines for Scan Parameters client library */

/* GATT Invalid CID*/
#define INVALID_CID (0xffff)

/* Invalid start handle */
#define INVALID_GATT_START_HANDLE   (0xffff)

/*  Invalid end handle */
#define INVALID_GATT_END_HANDLE   (0x0)

/* Invalid Scan Interval Window handle*/
#define INVALID_SCAN_INTERVAL_WINDOW_HANDLE (0xffff)

/* Invalid Scan Refresh handle*/
#define INVALID_SCAN_REFRESH_HANDLE (0xffff)

/* Scan interval window characteristics  UUID as defined in bluetooth.org */
#define SCAN_INTERVAL_WINDOW_CHAR_UUID   (0x2A4F)

/* Scan refresh characteristics  UUID as defined in bluetooth.org */
#define SCAN_REFRESH_CHAR_UUID   (0x2A31)

/* Enable scan refresh notification */
#define ENABLE_SCAN_REFRESH_NOTIFICATION 0x01

/* Scan refresh required */
#define SCAN_REFRESH_REQUIRED   0

/* Check Init Input prams are valid */
#define INPUT_PARAM_NULL(app_task,scan_params_client,init_params) (app_task == NULL) || (scan_params_client == NULL ) ||(init_params == NULL)

/* Macro to check init params are valid or not */
#define CLIENT_INIT_PARAM_INVALID(param) ((param != NULL) && (param->cid ==INVALID_CID ||param->start_handle == INVALID_GATT_START_HANDLE || param->end_handle == INVALID_GATT_END_HANDLE ))

/* Enum For LIB internal messages */
typedef enum
{
    SCAN_PARAMS_CLIENT_INTERNAL_MSG_BASE = 0,
    SCAN_PARAMS_CLIENT_INTERNAL_MSG_SET_SCAN,      /* For a updating scan parameters */
    SCAN_PARAMS_CLIENT_INTERNAL_MSG_TOP            /* Top of message */
}scan_params_client_internal_msg_t;

#endif /* GATT_SCAN_PARAMS_CLIENT_PRIVATE_H */

