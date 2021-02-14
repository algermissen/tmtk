/*
 * $Id: util.c,v 1.8 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "tmlog.h"
#include "tmassert.h" 


void tm_log(void *user_data,TMLogEvent e, ...)
{
	va_list args;
	assert(user_data == NULL); /* as long as we don;t use it */
 
       	va_start(args,e);


	switch(e) {
	case TM_LOG_UNKNOWN_RDF_PREDICATE:
		vfprintf(stderr,"[WARNING] RDF predicate not known (%s)\n",args);
		break;
	case TM_LOG_PARSE:
		vfprintf(stderr,"[PARSE]   Element URI: %s\n",args);
		break;
#if 0
	case TM_LOG_NODEMERGE:
       		fprintf(stderr,"<LOG: processing %s>\n", va_arg(args, char *) );
		/*
			va_arg(args,TMTopic), va_arg(args,TMTopic));
		*/
		break;
#endif
	default:
		fprintf(stderr,"unknown LOG EVENT %d\n", e);
	}

       	va_end(args);
}

