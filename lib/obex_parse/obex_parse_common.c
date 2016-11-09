/****************************************************************************
Copyright (c) 2010 - 2015 Qualcomm Technologies International, Ltd.
Part of ADK 4.0

FILE NAME
    obex_parse_common.c

DESCRIPTION
    This file defines all common internal functions used by parse and 
    decode functionalities.
*/



#include "obex_parse_internal.h"

/*************************************************************************
 * NAME
 *  opSkipToTag
 *
 * DESCRIPTION 
 * Proceed to the beginning of the tag
 ************************************************************************/
const char* opSkipToTag( const char* s, 
                          const char* e, 
                          opType type )
{
    char start = (type == op_xml_object)? '<' : 'B';  

    while( (s != e) && (*s !=  start )) ++s;
    return s;
}

