/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    display_example.h

DESCRIPTION
    
    
NOTES
   
*/

#ifndef _DISPLAY_EXAMPLE_H_
#define _DISPLAY_EXAMPLE_H_

/* display plugin functions*/
void DisplayExamplePluginInit( DisplayExamplePluginTaskdata *task, Task app_task ) ;
void DisplayExamplePluginSetState( DisplayExamplePluginTaskdata *task, bool state ) ;
void DisplayExamplePluginSetText( DisplayExamplePluginTaskdata *task, char* text, u8 line, u8 text_length, u8 scroll, bool flash, u16 scroll_update, u16 scroll_pause, u16 display_time ) ;
void DisplayExamplePluginSetVolume( DisplayExamplePluginTaskdata *task, u16 volume ) ;
void DisplayExamplePluginSetIcon( DisplayExamplePluginTaskdata *task, u8 icon, bool state ) ;
void DisplayExamplePluginSetBattery( DisplayExamplePluginTaskdata *task, u8 battery_level ) ;

/*internal plugin message functions*/
void DisplayExamplePluginScrollText( DisplayExamplePluginTaskdata *task, DispExScrollMessage_T * dispscrmsg ) ;
void DisplayExamplePluginClearText( DisplayExamplePluginTaskdata *task, u8 line ) ;

#endif /*_DISPLAY_EXAMPLE_H_*/
