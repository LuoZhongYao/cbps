/****************************************************************************
Copyright (c) 2005 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0
*/

#ifndef AGHFP_CALL_H_
#define AGHFP_CALL_H_


#include "aghfp.h"


void aghfpSendRingAlert(AGHFP *aghfp);
void aghfpHandleSendRingAlert(AGHFP *aghfp);
void aghfpSendCallerId(AGHFP *aghfp, u8 type, u16 size_number, u16 size_string, u8 *data);
void aghfpHandleSendCallerId(AGHFP *aghfp, u8 type, u16 size_number, u16 size_string, u8 *data);
void aghfpHandleAnswer(AGHFP *aghfp);											/* ATA */
void aghfpSendInBandRingToneEnable(AGHFP *aghfp, bool enable);
void aghfpHandleInBandRingToneEnable(AGHFP *aghfp, bool enable);
void aghfpHandleCallHangUpReq(AGHFP *aghfp);									/* AT+CHUP */
void aghfpHandleDialReq(AGHFP *aghfp, u8 *number, u16 number_len);
void aghfpHandleMemoryDialReq(AGHFP *aghfp, u8 *number, u16 number_len);
void aghfpHandleLastNumberRedialReq(AGHFP *aghfp);								/* AT+BLDN */
void aghfpHandleCallHoldReq(AGHFP *aghfp, u16 action, u16 index);	        /* AT+CHLD=d */
void aghfpSendInBandRingTone(AGHFP *aghfp);


#endif /* AGHFP_CALL_H_ */
