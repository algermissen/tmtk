#include "tmtk.h"
#include "tmutil.h"
#include "tmassert.h"
#include <stdio.h>

#define __MAXLINE__ 1024

/**
 * check, if the pattern fragment is found in str. cs stands for
 * 'match case sensitiv', so set to 1 if that's what you desire.
 * 20.02.2000
 */

int contains(const char *strarg, const char *fragarg,int cs)
{
	int plen,flen,i,j;
	int match = 0;
	char *p,*f;	
	char str[__MAXLINE__];
	char fragment[__MAXLINE__];
	
	char str_cpy[__MAXLINE__], fragment_cpy[__MAXLINE__];

	strcpy(str,strarg);
	strcpy(fragment,fragarg);
	

	plen = strlen(str);
	flen = strlen(fragment);

		/* fragment bigger that phrase, can't be inside */	
	if(plen < flen)
		return 0;


	/* 
	 * if not case-sensitive, we have to make local copy, 
	 * and change upper case letters to lower case
	 * so we do not have to check for upper case each time !
	 */

	if(cs) { 
		p = str;
		f = fragment;
	} else {
		strcpy(str_cpy,str);
		strcpy(fragment_cpy,fragment);
			/* TODO: check for letters above ASCII 127 (ÄÜÖ...) */	
		for(p = str_cpy; *p != '\0'; p++) {
			if( (*p >= 'A') && (*p <= 'Z'))
				*p += 32;	
		}
		p = str_cpy;

		for(f = fragment_cpy; *f != '\0'; f++) {
			if( (*f >= 'A') && (*f <= 'Z'))
				*f += 32;	
		}
		f = fragment_cpy;
	}

		

	for(i = 0; i <= plen - flen; i++) {
		match = 1;
		for(j = 0; j < flen; j++) {
		
			if( (p[i+j] != f[j]) ) {
				match = 0;
				break;
			}
		}
		if(match)
			break;	
	}


	return match;
}


/**
 * check if s starts with f. This is always done case insensitive
 * (this is used for the KwicTextfiels for Update Program)
 * 20.02.2000
 */

int begins_with2(const char *s, const char *f)
{
	const char *ps, *pf;

	pf = f;
	ps = s;


	while( (*pf != '\0') ) {
		if(*ps == '\0') return 0;       /* too short */
		if(*ps != *pf) return 0;
		/*
		if(*ps != *pf) {
			if( (*ps >= 'A' && *ps <= 'Z') && (*ps+32 != *pf)) return 0;
			else if( (*pf >= 'A' && *pf <= 'Z') && (*pf+32 != *ps)) return 0;
		}
		*/
		ps++;
		pf++;
	}

	return 1;       /* yes, s starts with f */
}       


int begins_with(const char *strarg, const char *fragarg,int cs)
{
	int plen,flen,j;
	int match = 0;
	char *p,*f;		
	char str_cpy[__MAXLINE__], fragment_cpy[__MAXLINE__];
	char str[__MAXLINE__];
	char fragment[__MAXLINE__];
	
	strcpy(str,strarg);
	strcpy(fragment,fragarg);
	

	plen = strlen(str);
	flen = strlen(fragment);

		/* fragment bigger that phrase, can't be inside */	
	if(plen < flen)
		return 0;


	/* 
	 * if not case-sensitive, we have to make local copy, 
	 * and change upper case letters to lower case
	 * so we do not have to check for upper case each time !
	 */

	if(cs) { 
		p = str;
		f = fragment;
	} else {
		strcpy(str_cpy,str);
		strcpy(fragment_cpy,fragment);
			/* TODO: check for letters above ASCII 127 (ÄÜÖ...) */	
		for(p = str_cpy; *p != '\0'; p++) {
			if( (*p >= 'A') && (*p <= 'Z'))
				*p += 32;	
		}
		p = str_cpy;

		for(f = fragment_cpy; *f != '\0'; f++) {
			if( (*f >= 'A') && (*f <= 'Z'))
				*f += 32;	
		}
		f = fragment_cpy;
	}

		

	match = 1;

	for(j = 0; j < flen; j++) {
		if( (p[j] != f[j]) ) {
			match = 0;
			break;
		}
	}

	return match;
}


/**
 * xx.
 * yy.
 */


int srb_match(const char *str,const char *pattern,int flags)
{
	/*
	while( *s != '\0' && *pattern != '\0')
	{
		if(*s != *pattern)
		return 0;
		s++;
		pattern++;
	}
 
	if(*pattern != '\0')
		return 0;
	else
		return 1;

	*/
	return 0;

}






/**
 * Convert an ANSI character to lowercase.
 * skhdsjkdh
 */

int ansiToLower(int c)
{
	if(c >= 65 && c <= 90)
		return c + 32;

	if(c >= 192 && c <= 214)
		return c + 32;

	if(c >= 216 && c <= 222)
		return c + 32;

	if(c == 140)
		return 156;

	if(c == 138)
		return 154;

	if(c == 159)
		return 255;

	return c;	
}

/**
 * Perform a case insensitive strcmp.
 * Using the Ansi ASCII extension
 */

int srb_strequal(const char *s1,const char* s2,int cs)
{

	if(cs) {
		while( *s1 != '\0' && *s2 != '\0')
		{
			if(*s1 != *s2)
				return 0;
			s1++; s2++;
		} 

		if(*s1 != '\0' || *s2 != '\0')
			return 0;

		return 1;
	} else {
		while( *s1 != '\0' && *s2 != '\0')
		{
			if(ansiToLower(*s1) != ansiToLower(*s2))
				return 0;
			s1++; s2++;
		} 

		if(*s1 != '\0' || *s2 != '\0')
			return 0;

		return 1;
	}

}

/* FIXME: all implementation and also consider locale and Unicode stuff!!!,
maybe we ned encoding as a param? */
const char * tm_strstr(const char *haystack,const char* needle,int cs)
{
	int nlen;
	int hlen;
	const char *h;
	if(cs == TM_CASE_SENSITIVE)
		return( strstr(haystack,needle) );

	hlen = strlen(haystack);
	nlen = strlen(needle);

	for(h = haystack; *h; h++,hlen--)
	{
		const char *p,*q;
		int equal = 1;	/* assume */
		if(hlen < nlen)
			return NULL;
		p = h;
		q = needle;

		while( *q )
		{
			if(ansiToLower(*p) != ansiToLower(*q))
			{
				equal = 0;
				break;
			}
			p++; q++;
		} 
		if(equal)
			return h;

	}
	return NULL;
}


/*@} */
