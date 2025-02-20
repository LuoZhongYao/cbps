/*************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
 Part of ADK 4.0

FILE : 
    pio_common.h

CONTAINS:
    Common PIO opperations used by libraries and applications

**************************************************************************/

#ifndef PIO_COMMON_H_
#define PIO_COMMON_H_

#define PIN_INVALID 0xFF

/*!
    @brief Define the types for the upstream messages sent from the Power
    library to the application.
*/
typedef enum
{
    pio_pull, 
    pio_drive
} pio_common_dir;


/****************************************************************************
NAME
    PioCommonSetPin
    
DESCRIPTION
    This function will drive/pull a PIO to the specified level
    
RETURNS
    TRUE if successful, FALSE otherwise
*/
bool PioCommonSetPin(u8 pin, pio_common_dir dir, bool level);


/****************************************************************************
NAME
    PioCommonGetPin
    
DESCRIPTION
    This function will configure a pin as input and attempt to read it. This
    will return TRUE if the pin is high, FALSE if the pin is low or could
    not be configured.
*/
bool PioCommonGetPin(u8 pin);


/****************************************************************************
NAME
    PioCommonGetPin
    
DESCRIPTION
    This function will configure a pin as input and attempt to debounce it. 
    This will return TRUE if successful, FALSE otherwise.
*/
bool PioCommonDebounce(u32 pins, u16 count, u16 period);


#endif /*PIO_COMMON_H_*/
