/***************************************************************************
Copyright 2004 - 2015 Qualcomm Technologies International, Ltd.
FILE NAME
    sink_debug.h
DESCRIPTION
*/
#ifndef _SINK_DEBUG_H_
#define _SINK_DEBUG_H_

#ifndef RELEASE_BUILD /*allows the release build to ensure all of the below are removed*/

/*The individual configs*/

#ifndef DO_NOT_DOCUMENT
#endif 

/*end of DO_NOT_DOCUMENT*/
/*The global debug enable*/ 

#ifndef LOG_TAG
#   define LOG_TAG __func__
#endif

#define DEBUG_PRINT_ENABLED

#ifdef DEBUG_PRINT_ENABLED

#   define DEBUG(tag,fmt, ...)  printf("D/%-8.8s : " fmt, tag, ##__VA_ARGS__)
#   define LOGD(fmt,...)        DEBUG(LOG_TAG, fmt, ##__VA_ARGS__)

#   define DEBUG_MAIN
#   define DEBUG_INQ
#   define DEBUG_BUT_MAN
#   define DEBUG_BUTTONS
#   define DEBUG_CALL_MAN
#   define DEBUG_MULTI_MAN
#   define DEBUG_AUDIO
#   define DEBUG_SLC
#   define DEBUG_DEV
#   define DEBUG_LP
#   define DEBUG_CONFIG
#   define DEBUG_LM
#   define DEBUG_LEDS
#   define DEBUG_PIO
#   define DEBUG_POWER
#   define DEBUG_TONES
#   define DEBUG_VOLUME
#   define DEBUG_STATES
#   define DEBUG_AUTH
#   define DEBUG_DIM
#   define DEBUG_A2DP
#   define DEBUG_LINKLOSS
#   define DEBUG_PEER
#   define DEBUG_PEER_SM
#   define DEBUG_INIT
#   define DEBUG_AVRCP
#   define DEBUG_AUDIO_PROMPTS
#   define DEBUG_FILTER_ENABLE
#   define DEBUG_CSR2CSR
#   define DEBUG_USB
#   define DEBUG_MALLOCx
#   define DEBUG_PBAP
#   define DEBUG_MAPC
#   define DEBUG_GAIA
#   define DEBUG_SPEECH_REC
#   define DEBUG_WIRED
#   define DEBUG_AT_COMMANDS
#   define DEBUG_GATT
#   define DEBUG_GATT_MANAGER
#   define DEBUG_BLE
#   define DEBUG_BLE_GAP
#   define DEBUG_GATT_CLIENT
#   define DEBUG_GATT_ANCS_CLIENT
#   define DEBUG_GATT_BATTERY_CLIENT
#   define DEBUG_GATT_HID_CLIENT
#   define DEBUG_GATT_DIS_CLIENT
#   define DEBUG_GATT_IAS_CLIENT
#   define DEBUG_GATT_SPC_CLIENT
#   define DEBUG_GATT_SERVICE_CLIENT
#   define DEBUG_GATT_HID_RC
#   define DEBUG_GATT_BATTERY_SERVER
#   define DEBUG_DUT
#   define DEBUG_DI
#   define DEBUG_DISPLAY
#   define DEBUG_SWAT
#   define DEBUG_FM
#   define DEBUG_INPUT_MANAGER
#   define DEBUG_IR_RC
#   define DEBUG_BAT_REP
#else
#   define DEBUG(x) 
#endif /*DEBUG_PRINT_ENABLED*/
    /* If you want to carry out cVc license key checking in Production test
   Then this needs to be enabled */
#   define CVC_PRODTEST
#else /*RELEASE_BUILD*/    
/*used by the build script to include the debug but none of the individual debug components*/
#   ifdef DEBUG_BUILD 
#       define DEBUG(x) {printf x;}
#   else
#       define DEBUG(x) 
#   endif
#endif

#ifdef DEBUG_PEER
#   define PEER_DEBUG(x) DEBUG(x)
#else
#   define PEER_DEBUG(x) 
#endif

#define INSTALL_PANIC_CHECKx
#define NO_BOOST_CHARGE_TRAPS
#undef SINK_USB
#define HAVE_VBAT_SEL
#define HAVE_FULL_USB_CHARGER_DETECTION

#endif /*_SINK_DEBUG_H_*/
