/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0
*/

#include "aghfp.h"
#include "aghfp_private.h"
#include "aghfp_parse.h"
#include "aghfp_receive_data.h"

#include <source.h>
#include <panic.h>

u16 parseSource(Source rfcDataIn, Task task);


/****************************************************************************
	Called when we get an indication from the firmware that there's more data 
	received and waiting in the RFCOMM buffer. Parse it.
*/
void aghfpHandleReceivedData(AGHFP *aghfp, Source source)
{
    u16 len = SourceSize(source);

    /* Only bother parsing if there is something to parse */
    while (len > 0)
    {
		/* Keep parsing while we have data in the buffer */
        if (!parseSource(source, &aghfp->task))
            break;

		/* Check we have more data to parse */
        len = SourceSize(source);
    }
}
