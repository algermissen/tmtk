/*
 * $Id: util.h,v 1.6 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef UTIL_H
#define UTIL_H
#include <stdio.h>   /* for FILE type */
#include "tmtk.h"
/*
#include "tmmodel.h"
*/
 
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup Utilities
 * \ingroup LibraryCore
 */
/** @{ */

TM_API(char *) tm_strdup(const char *s);
TM_API(char *) tm_strndup(const char *s, int len);
/*
TM_API(int) tm_strcmp_ci(copnst char *lhs,const char*rhs);
*/

/*
 * Does not modify s, simply returns pointer to first non-WS char
 */
TM_API(const char *) tm_lstrip(const char *s);
TM_API(char *) tm_rstrip(char *s);

TM_API(int) tm_is_whitespace_string(const char *s);
TM_API(int) tm_is_comment_string(const char *s, char c);

TM_API(char*) tm_tolowercase(const char *s, char *buf,size_t size);

TM_API(char *) tm_url_unescape(char *s);

TM_API(int) tm_chomp(char *buf,size_t n);

TM_API(char) *tm_strtok(const char **strp,const char *delims,char *buf, size_t size);
TM_API(void) tm_fmtprint(FILE *f, const char *s, int indent, int width, int start_indent);

TM_API(int) tm_strcmp_v(const void *lhs, const void *rhs);
TM_API(unsigned int) tm_strhash_v(const void *what);

TM_API(int) tm_strendswith(const char *haystack, const char *needle);


/** Check if a URI is absolute.
 */
TM_API(int) tm_uri_is_abs(const char *uri);


/** Normalize a URI. */
TM_API(int) tm_uri_normalize(const char *uri, char *buf, size_t size);


/** construct a URI from a document URI and an ID */ 
TM_API(int)
tm_uri_from_id(const char *doc_uri, const char *id, char *buf, size_t size);

/** Construct a URI from a base URI and a URI reference. */
TM_API(int)
tm_uri_from_ref(const char *doc_uri,const char *ref,char *buf,size_t size);

enum {
	TM_CASE_SENSITIVE,
	TM_CASE_INSENSITIVE
};
TM_API(const char *) tm_strstr(const char *haystack, const char *needle, int flag);

/** @} */

#ifdef __cplusplus
} // extern C
#endif


#endif
