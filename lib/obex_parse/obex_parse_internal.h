/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    obex_parse_internal.h

DESCRIPTION
    Internal header file for OBEX library 

*/

#ifndef OBEX_PARSE_INTERNAL_H_
#define OBEX_PARSE_INTERNAL_H_

#include "obex_parse.h"

typedef enum{
    op_new_entry = 0,       
    op_in_start_tag,     /* e.g: BEGIN: or < in xml*/
    op_in_get_tag,       /* e.g: VCARD   or  msg ../> or <msg> */
    op_in_get_element,   /* e.g: TYPE:EMAIL or xyz=mno */ 
    op_in_end_tag,       /* e.g: END:VCARD or </tag> or /> */  
    op_end,              /* Completed one element */
    op_error
}opState;

typedef enum{
    op_irda_object = op_vobj_element ,
    op_xml_object = op_xml_element
}opType; 

/* OBEX Parser Handle. Handle will be provided to the application after 
   parsing the partial data to call the parser  */ 
typedef struct{
    opState state:3;
    unsigned  openEnv:1; /* unfold the envelop */
    opType  type:4;
    unsigned folds:8;
} opHandle;

#define opSkipCRLF(s, e ) opSkipChars(s, e, '\n', '\r')
#define opSkipSpace(s, e ) opSkipChars(s, e, '\t', ' ' )
#define opSkipNULL( s,e )  opSkipChars(s, e, 0x0, '\v')
#define opSkipJunk(s, e) opSkipNULL( opSkipSpace( opSkipCRLF (s, e), e), e)
#define opSkipBlank(s, e) opSkipSpace( opSkipCRLF (s, e), e)

#define OP_IRDA_START_PROP_LEN      6   /* Length of BEGIN: */

const char* opGetNextElement(   opHandle *handle,
                                ObexParseData *element,
                                const char *s,
                                const char *e );

char *opDecode( ObexParseData *element,
                const char *tag,
                u16  tagLen,
                u16  *maxValLen );

bool opCheckStartTag(   const char *s,
                        const char *e, 
                        opType type );

/*************************************************************************
 * NAME
 *  opSkipToChar 
 *
 * DESCRIPTION 
 * Skip through the buffer till find the requested character
 ************************************************************************/
static __inline__ const char* opSkipToChar(const char* s, const char* e, char c )
{
    while( (s != e) && (*s != c )) ++s;
    return s;
}


/*************************************************************************
 * NAME
 *  opSkipToAnyChar 
 *
 * DESCRIPTION 
 * Skip through the buffer till find any of the requested character from 2. 
 ************************************************************************/
static __inline__ const char* opSkipToAnyChar( const char* s, 
                                        const char* e, 
                                        char x , 
                                        char y )
{
    while( (s != e) && (*s != x ) && (*s != y )  ) ++s;
    return s;
}

/*************************************************************************
 * NAME
 *  opSkipToMulChar 
 *
 * DESCRIPTION 
 * Skip through the buffer till find any of the request character from 3.
 ************************************************************************/
static __inline__ const char* opSkipToMulChars( const char* s, 
                                         const char* e, 
                                         char x , 
                                         char y,
                                         char z )
{
    while( (s != e) && (*s != x ) && (*s != y ) && (*s != z) ) ++s;
    return s;
}


/*************************************************************************
 * NAME
 *  opSkipChars 
 *
 * DESCRIPTION 
 * ignore the sequense of the requested charactes  
 ************************************************************************/
static __inline__ const char* opSkipChars( const char* s, 
                                    const char* e, 
                                    char c , 
                                    char d )
{
    while( (s != e) && (*s == c  || *s == d )) ++s;
    return s;
}



/*************************************************************************
 * NAME
 *  opCheckChar
 *
 * DESCRIPTION 
 * Check the next character is a predicted one 
 ************************************************************************/
static __inline__ const char* opCheckChar( const char *s, const char *e, char c )
{
    if(!s) return NULL;
    return ((s != e) && (*s == c ))? ++s : NULL; 
}


/*************************************************************************
 * NAME
 *  opAddElement
 *
 * DESCRIPTION 
 * Add a element to the Object Tree
 ************************************************************************/
static __inline__ void opAddElement(  ObexParseData* element, 
                               ObexParseFragment fragment,
                               ObexParseObject type,
                               const char* data,
                               u16  len )
{
    element->type = type;
    element->fragment = fragment;
    element->object = data;
    element->len = len;
} 


#endif /* OBEX_PARSE_INTERNAL_H_ */

