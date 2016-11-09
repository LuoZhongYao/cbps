/*
    Warning - this file was autogenerated by genparse
    DO NOT EDIT - any changes will be lost
*/

#ifndef AGHFP_PARSE_H
#define AGHFP_PARSE_H

#include <message_.h>

#ifdef __XAP__
#include <source.h>
#endif

const u8 *parseData(const u8 *s, const u8 *e, Task task);
void handleUnrecognised(const u8 *data, u16 length, Task task);

#ifdef __XAP__
u16 parseSource(Source rfcDataIncoming, Task task);
#endif

struct sequence
{
  const u8 *data;
  u16 length;
};

struct value_aghfpHandleAvailableCodecs_codecs
{
  u16 codec;
};
struct region_aghfpHandleAvailableCodecs_codecs
{
  u16 count;
  const u8 *s;
  const u8 *e;
  u16 next;
  const u8 *next_s;
};
struct value_aghfpHandleAvailableCodecs_codecs get_aghfpHandleAvailableCodecs_codecs(const struct region_aghfpHandleAvailableCodecs_codecs*, u16);

struct aghfpHandleDialParse
{
  struct sequence number;
};
struct aghfpHandleMemoryDialParse
{
  struct sequence number;
};
struct aghfpHandleAvailableCodecs
{
  struct region_aghfpHandleAvailableCodecs_codecs codecs;
};
struct aghfpHandleWbsCodecNegotiation
{
  u16 codec;
};
struct aghfpHandleBiaParse
{
  struct sequence indicators;
};
struct aghfpHandleVgmParse
{
  u16 gain;
};
struct aghfpHandleVgsParse
{
  u16 volume;
};
struct aghfpHandleVtsParse
{
  struct sequence code;
};
struct aghfpHandleBrsfReqParse
{
  u16 supportedFeatures;
};
struct aghfpHandleBtrhSetStatusParse
{
  u16 cmd;
};
struct aghfpHandleBvraParse
{
  u16 state;
};
struct aghfpHandleCcwaParse
{
  u16 state;
};
struct aghfpHandleChldParse
{
  struct sequence cmd;
};
struct aghfpHandleCkpdParse
{
  u16 keycode;
};
struct aghfpHandleClipParse
{
  u16 state;
};
struct aghfpHandleCmerReqParse
{
  u16 disp;
  u16 ind;
  u16 keyp;
  u16 mode;
};
struct aghfpHandleCopsFormatParse
{
  u16 format;
  u16 mode;
};
struct aghfpHandleNrecParse
{
  u16 state;
};
struct aghfpHandleFeatureNegotiation
{
  u16 ind;
  u16 val;
};
struct aghfpHandleFeatureNegotiationWithBandwidth
{
  u16 ind0;
  u16 ind1;
  u16 val0;
  u16 val1;
};
struct aghfpHandleReponseCSRSupportedFeaturesCodecsBw
{
  u16 battLevel;
  u16 callerName;
  u16 codecBandwidths;
  u16 codecs;
  struct sequence ignore;
  u16 pwrSource;
  u16 rawText;
  u16 smsInd;
};
struct aghfpHandleReponseCSRSupportedFeatures
{
  u16 battLevel;
  u16 callerName;
  u16 codecs;
  struct sequence ignore;
  u16 pwrSource;
  u16 rawText;
  u16 smsInd;
};
void aghfpHandleDialParse(Task , const struct aghfpHandleDialParse *);
void aghfpHandleMemoryDialParse(Task , const struct aghfpHandleMemoryDialParse *);
void aghfpHandleAtaParse(Task );
void aghfpHandleAvailableCodecs(Task , const struct aghfpHandleAvailableCodecs *);
void aghfpHandleWbsCodecConnection(Task );
void aghfpHandleWbsCodecNegotiation(Task , const struct aghfpHandleWbsCodecNegotiation *);
void aghfpHandleBiaParse(Task , const struct aghfpHandleBiaParse *);
void aghfpHandleVgmParse(Task , const struct aghfpHandleVgmParse *);
void aghfpHandleVgsParse(Task , const struct aghfpHandleVgsParse *);
void aghfpHandleVtsParse(Task , const struct aghfpHandleVtsParse *);
void aghfpHandleBldnParse(Task );
void aghfpHandleBrsfReqParse(Task , const struct aghfpHandleBrsfReqParse *);
void aghfpHandleBtrhStatusRequestParse(Task );
void aghfpHandleBtrhSetStatusParse(Task , const struct aghfpHandleBtrhSetStatusParse *);
void aghfpHandleBvraParse(Task , const struct aghfpHandleBvraParse *);
void aghfpHandleCcwaParse(Task , const struct aghfpHandleCcwaParse *);
void aghfpHandleChldSupportReqParse(Task );
void aghfpHandleChldParse(Task , const struct aghfpHandleChldParse *);
void aghfpHandleChupParse(Task );
void aghfpHandleCindStatusReqParse(Task );
void aghfpHandleCkpdParse(Task , const struct aghfpHandleCkpdParse *);
void aghfpHandleClccParse(Task );
void aghfpHandleClipParse(Task , const struct aghfpHandleClipParse *);
void aghfpHandleCmerReqParse(Task , const struct aghfpHandleCmerReqParse *);
void aghfpHandleCnumParse(Task );
void aghfpHandleCopsStatusParse(Task );
void aghfpHandleCopsFormatParse(Task , const struct aghfpHandleCopsFormatParse *);
void aghfpHandleNrecParse(Task , const struct aghfpHandleNrecParse *);
void aghfpHandleCindSupportReqParse(Task );
void aghfpHandleFeatureNegotiation(Task , const struct aghfpHandleFeatureNegotiation *);
void aghfpHandleFeatureNegotiationWithBandwidth(Task , const struct aghfpHandleFeatureNegotiationWithBandwidth *);
void aghfpHandleReponseCSRSupportedFeaturesCodecsBw(Task , const struct aghfpHandleReponseCSRSupportedFeaturesCodecsBw *);
void aghfpHandleReponseCSRSupportedFeatures(Task , const struct aghfpHandleReponseCSRSupportedFeatures *);
void aghfpHandleBinpParse(Task );

#endif
