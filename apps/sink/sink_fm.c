/****************************************************************************
Copyright (c) 2012 - 2016 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_fm.c

DESCRIPTION
    This file handles connection and stream setup for FM Rx
    (sink to plugin interfacing)
*/

#include <string.h>

#ifdef ENABLE_FM

#include <fm_rx_plugin.h>
#include <fm_rx_api.h>
#include <ps.h>

#include "sink_debug.h"
#include "sink_statemanager.h"
#include "sink_private.h"
#include "sink_config.h"
#include "sink_audio.h"

#ifdef ENABLE_SUBWOOFER
#include "sink_swat.h"
#endif

#ifdef ENABLE_DISPLAY
#include "sink_display.h"
#include "display_plugin_if.h"
#endif

#include "sink_fm.h"

#ifdef DEBUG_FM
    #define FM_ASSERT(x) { if (!(x)) { Panic(); } }
#else
    #define FM_ASSERT(x)
#endif

#define FM_PLUGIN_RX (TaskData *) &fm_rx_plugin

/****************************************************************************
NAME
    sinkFmInit

DESCRIPTION
    initialises the fm hardware, mode is passed to determine which part of the FM
    system is to be configured
*/
static void sinkFmInit(fm_mode mode)
{
    LOGD("sinkFmInit \n");

     switch (mode)
    {
        case FM_ENABLE_RX:
            fmRxInit(FM_PLUGIN_RX, &theSink.task, theSink.conf2->sink_fm_data.fm_plugin_data);
            break;

        case FM_ENABLE_TX:
        case FM_ENABLE_RX_TX:
        default:
            break;
    }
}

/****************************************************************************
NAME
    sinkFmRxAudioConnect

DESCRIPTION
    connects the I2S FM audio via the FM audio plugin which allows tones play
    and volume control
*/
void sinkFmRxAudioConnect(void)
{
    LOGD("sinkFmRxAudioConnect \n");

    /* Make sure we're using correct parameters for FM audio */
    theSink.a2dp_link_data->a2dp_audio_connect_params.mode_params = &theSink.a2dp_link_data->a2dp_audio_mode_params;

#ifdef ENABLE_SUBWOOFER
    /* set the sub woofer link type prior to passing to audio connect */
    theSink.a2dp_link_data->a2dp_audio_connect_params.sub_woofer_type  = AUDIO_SUB_WOOFER_NONE;
    theSink.a2dp_link_data->a2dp_audio_connect_params.sub_sink  = NULL;
    /* bits inverted in dsp plugin */
#endif

    LOGD("FM: Routing (Vol %d)\n", theSink.volume_levels->fm_volume.main_volume);

    AudioConnect((TaskData *)&csr_fm_decoder_plugin,
                 theSink.routed_audio,
                 AUDIO_SINK_FM,
                 theSink.codec_task,
                 VolumeConvertStepsToDB(theSink.volume_levels->fm_volume.main_volume,
                                             &sinkVolumeGetGroupConfig(multi_channel_group_main)),
                 FM_SAMPLE_RATE,
                 theSink.conf2->audio_routing_data.PluginFeatures,
                 AUDIO_MODE_CONNECTED,
                 AUDIO_ROUTE_INTERNAL,
                 powerManagerGetLBIPM(),
                 &theSink.a2dp_link_data->a2dp_audio_connect_params,
                 &theSink.task);

    audioControlLowPowerCodecs (FALSE) ;

    VolumeSetupInitialMutesAndVolumes(&theSink.volume_levels->fm_volume);

#ifdef ENABLE_SUBWOOFER
    /* set subwoofer volume level */
    updateSwatVolume(theSink.volume_levels->fm_volume.main_volume);
#endif
    /* tune to stored frequency after initilising the I2S interface otherwise FM receiver
       may be in an unknown state which may not work reliably */
    fmRxTuneFrequency(theSink.conf2->sink_fm_data.fmRxTunedFreq);
}


/****************************************************************************
NAME
    sinkFmRxAudioDisconnect

DESCRIPTION
    Disconnects the FM audio via the audio library/FM audio plugin
*/
void sinkFmRxAudioDisconnect(void)
{
    LOGD("sinkFmRxAudioDisconnect \n");

    /* ensure FM is on and FM audio currently being routed to speaker */
    if (sinkFmIsFmRxOn() && sinkFmAudioSinkMatch(theSink.routed_audio))
    {
        LOGD("FM audio disconnected \n");

        /* reset routed audio */
        theSink.routed_audio = 0;

        /* disconnect FM audio via audio library */
        AudioDisconnect();

        /* Update limbo state */
        if (stateManagerGetState() == deviceLimbo )
            stateManagerUpdateLimboState();
    }
}

/****************************************************************************
NAME
    sinkFmTuneUp

DESCRIPTION
    initiates an FM auto tune in an increasing frequency direction
*/
static void sinkFmRxTuneUp(void)
{
    LOGD("sinkFmRxTuneUp \n");
    fmRxTuneUp();
}


/****************************************************************************
NAME
    sinkFmTuneDown

DESCRIPTION
    initiates an FM auto tune in a decreasing frequency direction
*/
static void sinkFmRxTuneDown(void)
{
    LOGD("sinkFmRxTuneDown \n");
    fmRxTuneDown();
}

/****************************************************************************
NAME
    sinkFmDisplayFreq

DESCRIPTION
    utility function for displaying FM station on LCD
    A favourite station will be indicated by appending a (*)
*/
static void sinkFmDisplayFreq(u16 freq, fm_display_type type )
{
#if ENABLE_DISPLAY
    char display_freq[FM_DISPLAY_STR_LEN];
    u8 len;

    if (type == FM_ADD_FAV_STATION)
    {
        /*Add a star to a favourite station for the user to identify*/
        len = sprintf(display_freq, "%d.%d FM*", (freq/100), (freq%100)/10);
    }
    else
    {
        len = sprintf(display_freq, "%d.%d FM", (freq/100), (freq%100)/10);
    }

    LOGD("FM display freq: %s  (len = %d)\n", display_freq, len);

    displayShowText((char*) display_freq,  len,  1, DISPLAY_TEXT_SCROLL_STATIC, 500, 2000, FALSE, 0);
#endif
}

/****************************************************************************
NAME
    sinkFmGetIndex

DESCRIPTION
    utility function to get a index for requested operation
    In case of STORE, returns a free index.
    In case of ERASE, returns index corresponding to the requested freq

RETURNS
    index in PS key
*/
static u8 sinkFmGetIndex(fm_stored_freq *stored_freq, u16 freq)
{
    u8 index;

    for (index=0;index<FM_MAX_PRESET_STATIONS;index++)
    {
        if (stored_freq->freq[index] == freq)
        {
            break;
        }
    }

    LOGD("sinkFmGetIndex (%d)\n", index);

    return index;
}

/****************************************************************************
NAME
    sinkFmUpdateAtIndex

DESCRIPTION
    utility function to update freq in Ps key at requested index
*/
static void sinkFmUpdateAtIndex(u8 index, u16 freq, fm_stored_freq *stored_freq)
{
    FM_ASSERT(index<FM_MAX_PRESET_STATIONS);
    stored_freq->freq[index] = freq;
}

/****************************************************************************
NAME
    sinkFmRxStoreFreq

DESCRIPTION
    Stores the currently tuned frequency to persistant store, if storage is full
    signal to user
*/
static void sinkFmRxStoreFreq(u16 freq)
{
    fm_stored_freq *stored_freq = &theSink.conf2->sink_fm_data.fmStoredFreq;
    u8 index = FM_MAX_PRESET_STATIONS;

    LOGD("sinkFmRxStoreFreq freq %d\n", freq);

    if (freq != FM_INVALID_FREQ)
    {
        /*If requested freq already present in PSKEY, ignore the request*/
        if (sinkFmGetIndex(stored_freq, freq) < FM_MAX_PRESET_STATIONS)
        {
            LOGD("Freq already stored - Do nothing\n");
        }
        else
        {
            /*Get free index*/
            index = sinkFmGetIndex(stored_freq, FM_INVALID_FREQ);

            if (index < FM_MAX_PRESET_STATIONS)
            {
                LOGD("Stored station %d at index %d \n", freq, index);
                sinkFmUpdateAtIndex(index, freq, stored_freq);

                /* store requested frequency */
                (void) ConfigStore(CONFIG_FM_FREQUENCY_STORE, stored_freq, sizeof(fm_stored_freq));

                /*Display stored freq with a favourite sign*/
                sinkFmDisplayFreq(freq, FM_ADD_FAV_STATION);
            }
            else /*If no free index available, storage is full, indicate to user*/
            {
#ifdef ENABLE_DISPLAY
                displayShowText(DISPLAYSTR_FM_STORAGE_FULL,  strlen(DISPLAYSTR_FM_STORAGE_FULL), 1, DISPLAY_TEXT_SCROLL_SCROLL, 500, 2000, FALSE, 5);
#endif
                LOGD("FM storage full. Please delete a stored station. \n");
            }
        }
    }
}

/****************************************************************************
NAME
    sinkFmRxEraseFreq

DESCRIPTION
    erases the currently tuned frequency if it is stored in persistant store
*/
static void sinkFmRxEraseFreq(u16 freq)
{
    fm_stored_freq *stored_freq = &theSink.conf2->sink_fm_data.fmStoredFreq;
    u8 index = FM_MAX_PRESET_STATIONS;

    LOGD("sinkFmRxEraseFreq \n");

    /*Get index where requested freq is stored*/
    index = sinkFmGetIndex(stored_freq, freq);

    /*If no free index available, storage is full, indicate to user*/
    if (index < FM_MAX_PRESET_STATIONS)
    {
        LOGD("Erased station %d at index %d \n", freq, index);

        sinkFmUpdateAtIndex(index, FM_INVALID_FREQ, stored_freq);

        LOGD("Station tuned to Freq %d erased \n", freq);

        /* erase the stored frequency */
        (void) ConfigStore(CONFIG_FM_FREQUENCY_STORE, stored_freq, sizeof(fm_stored_freq));

        /*Display stored freq without a favourite sign, continue tuning until changed by user*/
        sinkFmDisplayFreq(freq, FM_DEL_FAV_STATION);
    }
}

/****************************************************************************
NAME
    sinkFmGetNextStoredFreq

DESCRIPTION
    utility function to get return the next stored freq in after the
    current frequency
*/
static u16 sinkFmGetNextStoredFreq(u16 curr_freq, fm_stored_freq *stored_freq)
{
    u16 firstValidFreq = FM_INVALID_FREQ;
    bool bFound = FALSE;
    u8 index = 0;

    while (index < FM_MAX_PRESET_STATIONS)
    {
        if (stored_freq->freq[index])/*check for valid freq in stored list*/
        {
            if (firstValidFreq == FM_INVALID_FREQ)
            {
                /*Store the first valid freq for cases where the curr_freq is not present in stored list*/
                firstValidFreq = stored_freq->freq[index];
            }

            if (stored_freq->freq[index] == curr_freq)
            {
                bFound = TRUE;
            }
            else if (bFound == TRUE)
            {
                return stored_freq->freq[index];
            }
        }
        index++;
    }

    return firstValidFreq;
}

/****************************************************************************
NAME
    sinkFmRxTuneToStore

DESCRIPTION
    tunes the FM receiver to the stored frequency in increasing order
    If currently tuned freq is the first stored freq, then the second entry in the stored list will be played.
    If the second entry is zero, the third entry will be tuned to.
    If no station is stored, request is ignored.
*/
static void sinkFmRxTuneToStore(u16 current_freq)
{
    u16 tune_freq = FM_INVALID_FREQ;
    fm_stored_freq *stored_freq = &theSink.conf2->sink_fm_data.fmStoredFreq;

    LOGD("sinkFmRxTuneToStore current freq %d \n", current_freq);

    /*Check if currently tuned freq is stored, then tune to the next entry*/
    tune_freq = sinkFmGetNextStoredFreq(current_freq, stored_freq);

    LOGD("Tune to freq %d \n", tune_freq);

    /* ensure valid frequency before attempting to tune to it */
    if(tune_freq!=FM_INVALID_FREQ)
        fmRxTuneFrequency(tune_freq);
    else
        MessageSend ( &theSink.task, EventSysError, 0) ;
}


/****************************************************************************
NAME
    sinkFmRxUpdateVolume

DESCRIPTION
    Sets the volume output of the FM reciever chip itself, codec volume control
    is via the FM audio plugin
*/
static void sinkFmRxUpdateVolume(u8 vol)
{
    LOGD("sinkFmRxUpdateVolume \n");
    fmRxUpdateVolume(vol * FM_VOLUME_SCALE_FACTOR);
}

/****************************************************************************
NAME
    sinkFmAudioSinkMatch

DESCRIPTION
    Compare sink to FM audio sink.

RETURNS
    TRUE if Sink matches, FALSE otherwise
*/
bool sinkFmAudioSinkMatch(Sink sink)
{
    if(sinkFmIsFmRxOn())
        return sink == sinkFmGetFmSink();
    return FALSE;
}

/****************************************************************************
NAME
    sinkFmIsFmRxOn

DESCRIPTION
    Check to see if the FM receiver is on.
*/
bool sinkFmIsFmRxOn(void)
{
    return ((theSink.conf2->sink_fm_data.fmRxOn)?(TRUE):(FALSE));
}

/****************************************************************************
NAME
    sinkFmSetFmRxOn

DESCRIPTION
    Sets the FmRxOn flag.
*/
void sinkFmSetFmRxOn(bool isOn)
{
    theSink.conf2->sink_fm_data.fmRxOn = isOn;
}

/****************************************************************************
NAME
    sinkFmRxPowerOff

DESCRIPTION
    Power off the FM receiver.
*/
static void sinkFmRxPowerOff(void)
{
    LOGD("sinkFmRxPowerOff \n");
    fmRxPowerOff();
    /* store current frequency in ps */
    configManagerWriteSessionData() ;
}

/****************************************************************************
NAME
    processEventUsrFmRxInOnState

DESCRIPTION
    processes On state FM related user events.

RETURN
    TRUE if handled, FALSE otherwise
*/
static bool processEventUsrFmRxInOnState(const MessageId EventUsrFmRx)
{
    bool success = TRUE;
    switch(EventUsrFmRx)
    {
        case EventUsrFmRxOff:
            sinkFmRxAudioDisconnect();
            sinkFmRxPowerOff();
            sinkFmSetFmRxOn(FALSE);
#ifdef ENABLE_DISPLAY
            displayShowSimpleText(DISPLAYSTR_CLEAR,1);
            displayShowSimpleText(DISPLAYSTR_CLEAR,2);
#endif
            break;
        case EventUsrFmRxTuneUp:
            sinkFmRxTuneUp();
            break;
        case EventUsrFmRxTuneDown:
            sinkFmRxTuneDown();
            break;
        case EventUsrFmRxStore:
            sinkFmRxStoreFreq(theSink.conf2->sink_fm_data.fmRxTunedFreq);
            break;
        case EventUsrFmRxTuneToStore:
            sinkFmRxTuneToStore(theSink.conf2->sink_fm_data.fmRxTunedFreq);
            break;
        case EventUsrFmRxErase:
            sinkFmRxEraseFreq(theSink.conf2->sink_fm_data.fmRxTunedFreq);
            break;
        default:
            success = FALSE;
            break;
    }
    return success;
}

/****************************************************************************
NAME
    processEventUsrFmRxInOffState

DESCRIPTION
    processes Off state FM related user events.

RETURN
    TRUE if handled, FALSE otherwise
*/
static bool processEventUsrFmRxInOffState(const MessageId EventUsrFmRx)
{
    bool success = FALSE;
    if(EventUsrFmRx == EventUsrFmRxOn)
    {
        if(stateManagerGetState() != deviceLimbo)
        {
            sinkFmInit(FM_ENABLE_RX);
            success = TRUE;
        }
    }
    return success;
}

/****************************************************************************
NAME
    sinkFmProcessEventUsrFmRx

DESCRIPTION
    processes FM related user events.

RETURN
    TRUE if handled, FALSE otherwise
*/
bool sinkFmProcessEventUsrFmRx(const MessageId EventUsrFmRx)
{
    bool success = TRUE;
    if(sinkFmIsFmRxOn())
    {
        success = processEventUsrFmRxInOnState(EventUsrFmRx);
    }
    else
    {
        success = processEventUsrFmRxInOffState(EventUsrFmRx);
    }
    return success;
}

/****************************************************************************
NAME
    sinkFmHandleFmPluginMessage

DESCRIPTION
    processes messages received from the FM plugin.
*/
void sinkFmHandleFmPluginMessage(const MessageId id, const Message message)
{
    switch (id)
    {
        /* received when the FM hardware has been initialised and tuned to the
           last used frequency */
        case FM_PLUGIN_INIT_IND:
        {
            FM_PLUGIN_INIT_IND_T *m = (FM_PLUGIN_INIT_IND_T*) message;
            LOGD("HS: FM INIT: %d\n", m->result);

            if (m->result)
            {
                /* set the fm receiver hardware to default volume level (0x3F) */
                sinkFmRxUpdateVolume(theSink.volume_levels->fm_volume.main_volume);
                /* set flag to indicate FM audio is now available */
                sinkFmSetFmRxOn(TRUE);
                /* connect the FM audio if no other audio sources are avilable */
                audioHandleRouting(audio_source_none);
            }
        }
        break;

        /* received when tuning is complete, the frequency tunes to is returned
           within the message, this is stored in persistant store */
        case FM_PLUGIN_TUNE_COMPLETE_IND:
        {
            FM_PLUGIN_TUNE_COMPLETE_IND_T *m = (FM_PLUGIN_TUNE_COMPLETE_IND_T*) message;
            LOGD("HS: FM_PLUGIN_TUNE_COMPLETE_IND: %d\n", m->result);

            if (m->result)
            {
                /* valid the returned frequency and store for later writing to ps session data */
                if (m->tuned_freq!=0x0000)
                {
                    theSink.conf2->sink_fm_data.fmRxTunedFreq=m->tuned_freq;

                    /*Display new frequency, clear older display*/
#ifdef ENABLE_DISPLAY
                    {
                        /*If the freq is stored in the Ps key, add special char for user to identify as favourite station */
                        u8 index=0;
                        fm_display_type type=FM_SHOW_STATION;

                        for (index=0;index<FM_MAX_PRESET_STATIONS;index++)
                        {
                            if (theSink.conf2->sink_fm_data.fmStoredFreq.freq[index]==m->tuned_freq)
                            {
                                type=FM_ADD_FAV_STATION;
                                break;
                            }
                        }

                        /*Display frequency*/
                        sinkFmDisplayFreq(m->tuned_freq, type);
                    }
#endif
                }
                LOGD("FM RX currently tuned to freq (0x%x) (%d;\n", theSink.conf2->sink_fm_data.fmRxTunedFreq,
                                                                            theSink.conf2->sink_fm_data.fmRxTunedFreq));
            }
        }
        break;

#ifdef ENABLE_FM_RDS

        /*Display RDS info*/
        case FM_PLUGIN_RDS_IND:
        {
#ifdef ENABLE_DISPLAY
            FM_PLUGIN_RDS_IND_T *m = (FM_PLUGIN_RDS_IND_T*) message;
            LOGD("HS: FM_PLUGIN_RDS_IND \n");

            if ((m->data_len > 0) && (m->data != NULL))
            {
                LOGD("HS:  data from msg (%s) (%d) \n", m->data, m->data_len);

                switch (m->rds_type)
                {
                    case FMRX_RDS_PROGRAM_SERVICE:
                        {
                            LOGD("HS: PS data %s \n", m->data);
                            displayShowText((char*) m->data,  m->data_len,  2, DISPLAY_TEXT_SCROLL_STATIC, 500, 2000, FALSE, 0);
                        }
                        break;

                    case FMRX_RDS_RADIO_TEXT:
                        {
                            LOGD("HS: RT data %s \n", m->data);
                            displayShowText((char*) m->data,  m->data_len,  2, DISPLAY_TEXT_SCROLL_SCROLL, 500, 2000, FALSE, 50);
                        }
                        break;

                    case FMRX_RDS_PROGRAM_TYPE:
                        {
                            LOGD("HS: Program Type %s \n", m->data);
                            displayShowText((char*) m->data,  m->data_len,  2, DISPLAY_TEXT_SCROLL_SCROLL, 500, 2000, FALSE, 50);
                        }
                        break;

                    default:
                        break;
                }
            }

#endif
        }
        break;

#endif /*ENABLE_FM_RDS*/

        default:
        LOGD("HS :  FM unhandled msg [%x]\n",id);
        break ;
    }
}

/****************************************************************************
NAME
    sinkFmSetFmDataFromSessionData

DESCRIPTION
    Sets FM related data using stored session data values.
*/
void sinkFmSetFmDataFromSessionData(const session_data_type * const session_data)
{
    theSink.conf2->sink_fm_data.fmRxTunedFreq = session_data->fm_frequency;
    theSink.volume_levels->fm_volume = session_data->fm_volume;
}

/****************************************************************************
NAME
    sinkFmSetSessionDataFromFmData

DESCRIPTION
    Sets FM related session data using using current data values.
*/
void sinkFmSetSessionDataFromFmData(session_data_type * const session_data)
{
    session_data->fm_frequency      = theSink.conf2->sink_fm_data.fmRxTunedFreq;
    session_data->fm_volume         = theSink.volume_levels->fm_volume;
}

/****************************************************************************
NAME
    sinkFmReadConfig

DESCRIPTION
    Allocate memory and retrieve FM config from PS
*/
void sinkFmReadConfig(void)
{
    int lSize = (sizeof(fm_rx_data_t) + FMRX_MAX_BUFFER_SIZE);
    theSink.conf2->sink_fm_data.fm_plugin_data = mallocPanic( lSize );
    memset(theSink.conf2->sink_fm_data.fm_plugin_data, 0, lSize);
    ConfigRetrieve(CONFIG_FM_CONFIG, &theSink.conf2->sink_fm_data.fm_plugin_data->config, sizeof(fm_rx_config));
    ConfigRetrieve(CONFIG_FM_FREQUENCY_STORE, &theSink.conf2->sink_fm_data.fmStoredFreq, sizeof(fm_stored_freq));
}

#endif /*ENABLE_FM*/
