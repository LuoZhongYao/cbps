/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0 

FILE NAME
    obex_object_handler.c

DESCRIPTION
    Internal function to encode and decode the OBEX Objects
*/


#include "obex_extern.h"
#include "obex_private.h"
#include <stdlib.h>
#include <sink.h>
#include <stream.h>
#include <source.h>
#include <memory.h>

/***************************************************************************
* NAME
*   obexObjFlush
*
* DESCRIPTION
* Flush an Object from the packet, Set out to TRUE to flush it to sink, 
* otherwise it drops the packet. 
***************************************************************************/
void obexObjFlush( Obex session, bool out )
{
    if( out ) SinkFlush( session->sink , session->pktLen );
    OBEX_OBJ_DELETE( session );
}

/***************************************************************************
* NAME
*  obexFetchHeader
*
* DESCRIPTION
*  Fetch a header in the packet
*
* RETURNS
*   Offset to the packet or invalid length 
****************************************************************************/
u16 obexFetchHeader(const u8* pkt, u16 len, u8 opcode)
{
    u16 processLen = 0;
    
    while( len > processLen )
    {
        if( pkt[processLen] == opcode ) return processLen;

        if( IsByteHdr(pkt[processLen]) )
        {
            /* Skip 1 Byte Header */
            processLen +=  OBEX_BYTE_HDR_SIZE;
        }
        else if( IsUintHdr( pkt[processLen]) )
        {
            /* Skip 4 byte header */
            processLen += OBEX_UINT_HDR_SIZE;
        }
        else
        {
            /* Skip a sequence header */
           u16 hdrLen;

           OBEX_UINT8_TO_UINT16( hdrLen, pkt, (processLen+1));
           processLen +=  hdrLen;
        }
    }

    return OBEX_INVALID_UINT16;
}


/***************************************************************************
* NAME
*   obexAddUint32Header
*
* DESCRIPTION
*   Add a Uint32 Header
****************************************************************************/
bool obexAddUint32Header( Obex session, u8 opcode, u32 value )
{
    u8* pkt = SinkMap( session->sink )+ session->pktLen;

    if(!obexSinkClaim( session, OBEX_UINT_HDR_SIZE )) return FALSE;

    pkt[0] = opcode;
    pkt[1] = (value >> 24) & 0xFF;
    pkt[2] = (value >> 16) & 0xFF;
    pkt[3] = (value >> 8) & 0xFF;
    pkt[4] = value & 0xFF;

    session->pktLen += OBEX_UINT_HDR_SIZE;

    return TRUE;
}


/***************************************************************************
* NAME
*   obexAddUint8Header
*
* DESCRIPTION
*   Add a Uint8 Header
****************************************************************************/
bool obexAddUint8Header( Obex session, u8 opcode, u8 value )
{
    u8* pkt = SinkMap( session->sink )+ session->pktLen;

    if(!obexSinkClaim( session, OBEX_BYTE_HDR_SIZE )) return FALSE;

    pkt[0] = opcode;
    pkt[1] = value;

    session->pktLen += OBEX_BYTE_HDR_SIZE;

    return TRUE;
}

/***************************************************************************
* NAME
*   obexGetUint32Header
*
* DESCRIPTION
*   Extract a Uint32 Header
****************************************************************************/
u32 obexGetUint32Header( const u8* pkt, u16 len, u8 opcode )
{
    u16 offset;

    offset = obexFetchHeader( pkt, len, opcode );
    
    if( offset == OBEX_INVALID_UINT16 )  return OBEX_INVALID_UINT32;

    return ((((u32)pkt[offset+1] << 24) & 0xFF000000) | 
            (((u32)pkt[offset+2] << 16) & 0x00FF0000) | 
            (((u32)pkt[offset+3] << 8) & 0x0000FF00)  | 
            ((u32)pkt[offset+4] & 0x000000FF));
}


/***************************************************************************
* NAME
*   obexGetUint8Header
*
* DESCRIPTION
*   Extract a Uint8 Header
****************************************************************************/
u8 obexGetUint8Header( const u8* pkt, u16 len, u8 opcode )
{
    u16 offset;

    offset = obexFetchHeader( pkt, len, opcode );
    
    if( offset == OBEX_INVALID_UINT16 )  return OBEX_INVALID_UINT8;

    return pkt[offset+1];
}

/***************************************************************************
* NAME
*   obexGetSeqHeader
*
* DESCRIPTION
*  Extract a unicode or ByteSeq header 
****************************************************************************/
u8* obexGetSeqHeader( const u8* pkt, u16* len, u8 opcode )
{
    u16 offset;

    offset = obexFetchHeader( pkt, *len , opcode );
    
    if( offset == OBEX_INVALID_UINT16 )
    {
        *len = 0;
        return NULL;
    }
    else
    {
        OBEX_UINT8_TO_UINT16( *len, pkt, offset+1 );
        *len -= OBEX_SEQ_HDR_SIZE;
        return (u8*) &pkt[offset+OBEX_SEQ_HDR_SIZE];
    }
}


/***************************************************************************
* NAME
*   obexAddEmptyHeader
*
* DESCRIPTION
*   Claim space for a Headers header.
****************************************************************************/
bool obexAddEmptyHeader( Obex session, u8 hdr, u16 size )
{
    u16 hdrLen = OBEX_SEQ_HDR_SIZE + size;
    u8* dest = SinkMap( session->sink ) + session->pktLen;    

    if( !obexSinkClaim(session, hdrLen) ) return FALSE;

    dest[0] = hdr;
    if(size) memset( &dest[3], 0x30, size );  
    size += OBEX_SEQ_HDR_SIZE;
    OBEX_UINT16_TO_UINT8( dest, size, 1 );

    session->pktLen += hdrLen;

    return TRUE;
}
/*************************************************************************
* NAME
*   obexAddSeqHeader
*
* DESCRIPTION
*   Add a Byte Sequence Header
*************************************************************************/
bool obexAddSeqHeader( Obex session, u8 hdr, u16 size, Source src )
{
    u8* dest = SinkMap( session->sink ) + session->pktLen;   
 
    if( (obexSinkSlack( session ) < (size + OBEX_SEQ_HDR_SIZE))) return FALSE;
    if( !obexAddEmptyHeader( session, hdr, 0 ) ) return FALSE;

    if( obexSinkSpace( session ) )
    {
        /* Already some claimed sink space available */
        obexSinkClaim( session , size );
        memmove( &dest[3], SourceMap(src), size );
    }
    else
    {
        StreamMove( session->sink, src, size );
    }

    session->pktLen += size; 
    size += OBEX_SEQ_HDR_SIZE;
    OBEX_UINT16_TO_UINT8( dest, size, 1 );

    return TRUE; 
}


/*************************************************************************
* NAME
*   obexAddUnicodeHeader
*
* DESCRIPTION
*   Add a Unicode Header
*************************************************************************/
bool obexAddUnicodeHeader( Obex session, u8 hdr, u16 size, Source src )
{
    u8* dest = SinkMap( session->sink ) + session->pktLen;   
    u8 i=0, j=1 ;
    u8* unicode= &dest[3];
    const u8* str = SourceMap( src );
    u16 unicodeLen = size * 2;

    if( str[size-1] != '\0' ) unicodeLen += 2;
 
    if( (obexSinkSlack( session ) < (unicodeLen + OBEX_SEQ_HDR_SIZE)))
    {
         return FALSE;
    }

    if( !obexAddEmptyHeader( session, hdr, 0 ) ) return FALSE;

    obexSinkClaim( session , unicodeLen );
    memset(unicode, 0, unicodeLen );

    for(i=0; i< size; i++, j+=2)
    {
        unicode[j] = str[i];  
    }
    unicode[unicodeLen-1] = '\0';

    session->pktLen += unicodeLen; 
    unicodeLen += OBEX_SEQ_HDR_SIZE;
    OBEX_UINT16_TO_UINT8( dest, unicodeLen, 1 );

    return TRUE; 
}

/**************************************************************************
* NAME
*  obexGetDigest
*
* DESCRIPTION
*  Validate and Retrieve the Digest String. 
*
* RETURNS
*  pointer to the Digest challenge string and its length
***************************************************************************/
const u8* obexGetDigest( Obex Session, 
                            const u8* pkt,
                            u16 *len,
                            u8 hdrId,
                            u8 tag )
{
    u16 valLen = 0;

    pkt = obexGetSeqHeader( pkt, len, hdrId  );

    valLen = *len;
    if( obexGetTLVHeaderValue( pkt, &valLen, OBEX_REQ_NONCE_TAG ) )
    {
        if( valLen == OBEX_SIZE_DIGEST) return pkt;
    }
        
    /* failed to validate the challenge string */
    *len = 0;
    return NULL;
}

/**************************************************************************
* NAME
*  obexGetTLVHeader
*
* DESCRIPTION
* Extract the Value from a TLV header like Digest or App params. 
***************************************************************************/
const u8* obexGetTLVHeaderValue( const u8* pkt,
                                    u16 *pktLen,
                                    u8  tag ) 
{
    u16 offset = 0;
    u16 dataOffset;

    /*
     *  --------------------------------------
     *  | Type      |   Length  |   Value -----------  
     *  --------------------------------------
     */
    while( (dataOffset = offset + OBEX_TLV_HDR_SIZE) < *pktLen )
    {
        if( ( pkt[offset] == tag ) &&
            ( pkt[offset + 1] <= ( *pktLen - dataOffset ) ) )
        {
            /* found and validated the tag */
            *pktLen = pkt[offset+1];
            return &pkt[offset+2];
        } 

        offset += pkt[offset + 1] + OBEX_TLV_HDR_SIZE;
    }
   
    *pktLen = 0; 
    return NULL;
}

/**************************************************************************
* NAME
*  obexFrameTLVHeader
*
* DESCRIPTION
*  Frame a TLV Header
***************************************************************************/
u16 obexFrameTLVHeader(  u8* pkt,
                            u8  tag,
                            u16 len,
                            const u8* data)
{
    pkt[0] = tag;
    pkt[1] = len;
    memmove( &pkt[2], data, len );
    return len + OBEX_TLV_HDR_SIZE;
}

