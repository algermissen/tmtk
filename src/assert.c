#include "tmtk.h"
#include "tmassert.h"

#include <stdio.h>

void tm_assert(char *file,unsigned line)
{
	fflush(NULL);
	fprintf(stderr,"\nAssertion failed: %s, line %u\n"
			"Package: "PACKAGE_STRING"\n"
			"Please report this message and as much information about\n"
			"the circumstances to "PACKAGE_BUGREPORT"\n\n"
			"Thank you very much!\n\n",
			file,line);
	fflush(stderr);
	abort();
}
