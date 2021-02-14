/*
 * $Id$
 *
 * Copyright (c) 2002,2003 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#include <tmproperty.h>
/*
struct TMProperty {
        const char *model;
        const char *name;
        TMValueType value_type;
        int type;
        const char *description;
        const char *fullname;
        const char *combination_code;
        const char *equality_code;
};
*/

struct TMProperty a_sidp = {
	"core",
	"a-sidp",
	&ASIDP,
	TM_SIDP,
	"Bla",
	"core::a-sidp",
	NULL,
	NULL
};

