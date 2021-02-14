#ifndef TM_ASSERT
#define TM_ASSERT 1
/*
#include <assert.h>
*/

#ifdef assert
#undef assert
#endif

#ifndef NDEBUG

void tm_assert(char *,unsigned);

#define assert(f) \
	do { \
		if(f) {} \
		else tm_assert(__FILE__,__LINE__); \
	} while(0);

#else

#define assert(f) 	/* as nothing */



#endif



#endif
