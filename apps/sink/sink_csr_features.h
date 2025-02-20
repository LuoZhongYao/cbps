/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.

FILE NAME
    sink_csr_features.h      

DESCRIPTION
    handles the csr to csr features 

NOTES

*/
/*!

@file	sink_csr_features.h
@brief csr 2 csr specific features    
*/


#ifndef _CSR2CSRFEATURES_

void csr2csrHandleTxtInd(void);
void csr2csrHandleSmsInd(void);   
void csr2csrHandleSmsCfm(void);
void csr2csrHandleAgBatteryRequestInd(void);
void csr2csrHandleAgBatteryRequestRes(u8 idx);
    
#endif
