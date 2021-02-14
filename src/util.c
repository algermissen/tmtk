/*
 * $Id: util.c,v 1.8 2002/10/12 19:05:57 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include "tmutil.h" 
#include "tmerror.h" 
#include "tmtrace.h"
#include "tmmem.h"
#include "tmassert.h"

#include <ctype.h>

int tm_chomp(char *buf,size_t n)
{
	assert(buf);
        while(buf[n-1] == '\n' || buf[n-1] == '\r')
                buf[--n] = '\0';
	return n;
}




char *tm_tolowercase(const char *s, char *buf,size_t size)
{
	char *p = buf;
	assert(s);
	assert(buf);
	while(*s && size) { *p = tolower(*s); p++,s++,size--; }
	*p = '\0';
	return buf;
}

char x2c(char *what)
{
	char digit;
	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10
				: (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10
				 : (what[1] - '0'));
	return digit;
}

char * tm_url_unescape(char *url)
{
	int x,y;
	for(x=0,y=0;url[y];++x,++y)
	{
		if( (url[x] = url[y]) == '%')
		{
			url[x] = x2c(&url[y+1]);
			y+=2;
		}
	}
	url[x] = '\0';
	return url;
}



int _is_in(char c, const char *delims)
{
	for ( ; *delims; delims++)
		if(c == *delims || ((c) < '!' || (c) > '~' ))
		{
			/*
			fprintf(stderr,"%c (%d) is in delims\n",c,c);
			*/
			return 1;
		}
	/*
	fprintf(stderr,"%c (%d) is not in delims\n",c,c);
	*/
	return 0;
}

char *tm_strtok(const char **strp,const char *delims,char *buf, size_t size)
{
	const char *p,*d;
	const char *end;
	const char *source = *strp;
	int match,len;
	/*
	fprintf(stderr,"source:    _%s_\n",source);
	*/


	while(*source && _is_in(*source,delims) )
		source++;
	/*
	fprintf(stderr,"source:    _%s_\n",source);
	*/

	end = p = source;

	if(!*p)
	{
		*buf = '\0';
		return NULL;
	}

	match = 0;
	while(*p && !match)
	{
		for (d = delims; *d; d++)
		{
			if(*p == *d  ||  ((*p) < '!' || (*p) > '~' ))
			{
				end = p;	
				while(*(p+1) && _is_in(*(p+1),delims) )
					p++;

				
				match = 1;
				break;
			}

		}
		p++;
		if(!match) end = p;
	}
	/*
	len = *p ? (p - source - 1) : (p - source);
	*/
	len = end - source;
	assert( len < size);
	strncpy(buf,source,len);
	buf[len] = '\0';
	/*
	fprintf(stderr,"word: _%s_\n",buf);
	*/
	*strp = p;
	return buf;
}


char *tm_strdup(const char *s)
{
	char *t;
	/* FIXME: allow this or not ?? */
	if(!s) return NULL;
	t = (char *)TM_ALLOC(strlen(s)+1);
	assert(t);
	strcpy(t,s);
	return t;
}
char *tm_strndup(const char *s, int len)
{
	char *t;
	/* FIXME: allow this or not ?? */
	if(!s) return NULL;
	t = (char *)TM_ALLOC(len+1);
	assert(t);
	memcpy(t,s,len);
	t[len] = '\0';
	return t;
}

void tm_fmtprint(FILE *f, const char *s, int indent, int width, int start_indent)
{
	int ind;
	int first = 1;
	char word[64];
	char *line;
	assert(s);
	line = TM_ALLOC((width+1) * sizeof(char));
	bzero(line, (width+1)*sizeof(char) );

	ind = start_indent;
	
	while( tm_strtok(&s," \t",word,sizeof(word)) != NULL)
	{
		assert(strlen(word) < width);	/* FIXME */
		
		if(strlen(line) + 1 + strlen(word) > width)
		{
			fprintf(f,"%*s%s\n", ind,"",line);
			bzero(line, (width+1)*sizeof(char) );
			if(first) { first = 0; ind = indent; }
		}		
		if(strlen(line) == 0)
			strcat(line,word);
		else
		{
			strcat(line," ");
			strcat(line,word);
		}
	}
	fprintf(f,"%*s%s\n", ind,"",line);
	
			
		
	TM_FREE(line);
}

int tm_strcmp_v(const void *lhs, const void *rhs)
{ return strcmp((const char*)lhs,(const char*)rhs); }
 
/* 
unsigned int tm_strhash_v(const void *what)
{
        char *s = (char *)what;
        unsigned long h = 0;
        while (*s)
                h = (h << 5) + h + (unsigned char)*s++;
        return h;
}
*/
unsigned int tm_strhash_v(const void *what)
{
        unsigned char *s;
        unsigned int h = 0;
	for(s=(unsigned char *)what; *s; s++)
                h = 31 * h + *s;
        return h;
}

int tm_strendswith(const char *haystack, const char *needle)
{
	int pos;
	pos = strlen(haystack) - strlen(needle);
	if(pos < 0)
		return 0;
	haystack += pos;
	return ( ! strcmp(haystack,needle));
}

/* FIXME: size checks and real normalization! */
int tm_uri_normalize(const char *uri, char *buf, size_t size)
{
	assert(uri);
	assert(buf);
	assert( tm_uri_is_abs(uri) );
	assert(size > (strlen(uri)+20) ); /* FIXME! */
	if(*uri == '/') {
		snprintf(buf,size,"file://%s",uri);
	} else {
		strcpy(buf,uri);
	}
	/* FIXME: retval */
	return 0;
}

/* FIXME: what does snprintf return ?? */
int tm_uri_from_id(const char *doc_uri, const char *id, char *buf,size_t size)
{
	int n;
	assert(doc_uri);
	assert(id);
	assert(buf);
	assert((strlen(doc_uri)+1+strlen(id)) < size);
	n = snprintf(buf,size,"%s#%s",doc_uri,id);
	return n;	
}

/* Most is FIXME */
int tm_uri_from_ref(const char *doc_uri,const char *ref,char *buf,size_t size)
{
	assert(doc_uri);
	assert(ref);
	assert(buf);

	if(*ref == '#') {
		snprintf(buf,size,"%s%s",doc_uri,ref);
	} else if(tm_uri_is_abs(ref)) {
		tm_uri_normalize(ref,buf,size);
	} else {
		char *p_last_slash = strrchr(doc_uri,'/');
		bzero(buf,size);
		/* doc uri MUST have file portion !!
		 * FIXME: check that is the setters !! */
		assert(p_last_slash);

		strncpy(buf,doc_uri, (p_last_slash - doc_uri + 1) );
		/* is 0 terminated, because we bzero-ed */
		strcat(buf,ref); /* FIXME: strNcat !! */
		/*
		fprintf(stderr,"-->_%s_\n",doc_uri);
		fprintf(stderr,"-->_%s_\n",href);
		fprintf(stderr,"-->_%s_\n",buf);
		*/
	}
	/* FIXME: retval? */
	return 0;	
}

int tm_uri_is_abs(const char *uri)
{
	assert(uri);
	if(*uri == '/')
		return 1;

	if(strstr(uri,"://") != NULL)
		return 1;

	return 0;
}


int tm_is_whitespace_string(const char *s)
{
	assert(s);
	while(*s)
	{
		if( ! isspace(*s) )
			return 0;
		s++;
	}
	return 1;	
}
int tm_is_comment_string(const char *s, char c)
{
	assert(s);
	while(*s)
	{
		if( ! isspace(*s))
		{
			if(*s == c)	/* is comment */ 
				return (1);
			else
				return 0;
		}
		s++;
	}
	return 1;	
}
const char *tm_lstrip(const char *str)
{
	assert(str);
	while( *str && isspace(*str) ) str++;
	return str;
}
/*
int tm_strcmp_ci(copnst char *lhs,const char*rhs)
{

}
*/


