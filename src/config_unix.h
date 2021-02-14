/*
 * $Id$
 *
 * Copyright (c) 2002,2003 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_CONFIG_H
#define TM_CONFIG_H

#define HAVE_SNPRINTF	1
#define HAVE_BZERO	1

#define TM_API(type) type

typedef unsigned int uint32;
typedef unsigned char byte;


/* works for gcc 2.4 and up */
/* FIXME: make this better! */
/*
#define TM_TRACEFUNCTION ((const char *) 0)
*/
#define TM_TRACEFUNCTION __PRETTY_FUNCTION__



#endif /* ifndef TM_CONFIG_H */

