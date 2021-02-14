my $last;

# This script generates src/base/err.c from src/includes/tmerror.h
# It is invoked by the Makefile.

print <<EOT;

/* machine generated, do not edit !! */

#include "tmerror.h"
#include "tmassert.h"

static const char *err_strings[] = {
EOT

while(<>) {
	#        TM_E_BADLOCATOR, /* invalid locator string */
	next unless(/^\s*([A-Z][A-Z]_[A-Z_0-9]+?)(,?)\s+\/\*\s+?([^\*]+?)\s+?\*\//);
	print "    \"$3\"$2  /* $1 */\n";
	$last = $1;
}



print <<EOT;

};

const char* tm_strerror(TMError e)
{
	assert(e >= 0 && e <= $last);
        return err_strings[e];
}

EOT

