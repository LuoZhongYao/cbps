/****************************************************************************
Copyright (c) 2004 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_indicators.h
    
DESCRIPTION
    
*/

#ifndef _SINK_INDICATORS_H_
#define _SINK_INDICATORS_H_


/****************************************************************************
NAME    
    indicatorsHandleServiceInd
    
DESCRIPTION
    Interprets the service Indicator messages and sends the appropriate message 

RETURNS
    void
*/
void indicatorsHandleServiceInd ( const HFP_SERVICE_IND_T *pInd );


#endif /* _SINK_INDICATORS_H_ */

