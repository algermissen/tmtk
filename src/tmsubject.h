/*
 * $Id: assertion.h,v 1.4 2002/09/04 20:53:04 jan Exp $
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#ifndef TM_SUBJECT_H
#define TM_SUBJECT_H

 
#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup TMSubject
 * \ingroup AbstractDataTypes
 *
 * @{
 */

/** A collection of property/value pairs.
 */
typedef struct TMSubject *TMSubject;


struct TMSubject {
	int N;
	struct {
		const char *model;
		const char *name;
		const void *value;
	} props[TM_MAXPROPERTIES];
};	



/** \def TM_STATIC_SUBJECT_N1(var,model,prop,value)
 * Create a static subject variable with one property as specified
 * by the arguments.
 *
 * FIXME: static is missing now to allow importing of subjects into other TMAs.
 */
#define TM_STATIC_SUBJECT_N1( var , model , prop , value ) \
	struct TMSubject var = { 1, { { model,prop,value} } }


/** \def TM_SET_SUBJECT_N1(s,m,p,v)
 * Assign one property to the TMSubject given by \a s
 */
#define TM_SET_SUBJECT_N1(s,m,p,v) \
	do { \
	(s)->N = 1; \
        (s)->props[0].model = (m); \
        (s)->props[0].name  = (p); \
        (s)->props[0].value = (v); \
	} while(0)


/** \def TM_SET_SUBJECT_N2(s,m1,p1,v1, m2,p2,v2)
 * Assign two properties to the TMSubject given by \a s
 */
#define TM_SET_SUBJECT_N2(s,m1,p1,v1, m2,p2,v2) \
	do { \
	(s)->N = 2; \
        (s)->props[0].model = (m1); \
        (s)->props[0].name  = (p1); \
        (s)->props[0].value = (v1); \
        (s)->props[1].model = (m2); \
        (s)->props[1].name  = (p2); \
        (s)->props[1].value = (v2); \
	} while(0)
 

/** @} */ 

#ifdef __cplusplus
} // extern C
#endif


#endif
