/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_pio.c

DESCRIPTION


*/

#include "sink_pio.h"

#include "sink_private.h"
#include "sink_configmanager.h"

#include <pio.h>
#include <psu.h>

#ifdef DEBUG_PIO
#else
#endif


/****************************************************************************
NAME
	PioSetPio

DESCRIPTION
    Fn to change a PIO
    Set drive pio_drive to drive PIO, pio_pull to pull PIO
    Set dir TRUE to set high, FALSE to set low

RETURNS
	void
*/
void PioSetPio(u16 pio, pio_common_dir drive, bool dir)
{
    LOGD("PIO: %s pin %d %s\n", (drive ? "Drive" : "Pull"), pio, (dir ? "high" : "low"));
    PioCommonSetPin(pio, drive, dir);
}


/****************************************************************************
NAME
	PioGetPio

DESCRIPTION
    Fn to read a PIO

RETURNS
	TRUE if set, FALSE if not
*/
bool PioGetPio(u16 pio)
{
    LOGD("PIO: Read pin %d\n", pio);
    return PioCommonGetPin(pio);
}


/****************************************************************************
NAME
	PioDrivePio

DESCRIPTION
	Drive a PIO to the passed state if configured. PIOs may be active high or
    active low depending on the pio_invert bitmap.

RETURNS
	void

*/
void PioDrivePio(u16 pio, bool state)
{
    if (pio != NO_PIO)
    {
        if (theSink.conf6->PIOIO.pio_invert & (1UL << pio))
        {
            state = !state;
        }

        PioSetPio(pio, pio_drive, state);
    }
}


/****************************************************************************
NAME
	PioSetPowerPin

DESCRIPTION
    controls the internal regulators to latch / remove the power on state

RETURNS
	void
*/
void PioSetPowerPin ( bool enable )
{
    /* power on or off the smps */
#ifdef BC5_MULTIMEDIA
    PsuConfigure(PSU_SMPS0, PSU_ENABLE, enable);
#else
    /* when shutting down ensure all psus are off for BC7 chips */
    if(!enable)
        PsuConfigure(PSU_ALL, PSU_ENABLE, enable);
#endif
    LOGD("PIO : PowerOnSMPS %X\n", enable);
}
