/*************************************************
 * Anthor  : LuoZhongYao@gmail.com
 * Modified: 2016 Nov 12
 ************************************************/
#ifndef __SINK_DEBUG_H__
#define __SINK_DEBUG_H__

#include <debug.h>

#ifdef DEBUG_PRINT_ENABLED

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

#endif

#define INSTALL_PANIC_CHECKx
#define NO_BOOST_CHARGE_TRAPS
#undef SINK_USB
#define HAVE_VBAT_SEL
#define HAVE_FULL_USB_CHARGER_DETECTION


#endif

