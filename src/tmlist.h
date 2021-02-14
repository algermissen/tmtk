/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */
#ifndef TM_LIST_INCLUDED
#define TM_LIST_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "tmtk.h"

/** \defgroup TMList
 * \ingroup AbstractDataTypes
 */
/** @{ */

/** \typedef TMList
 * A data type for linked lists.
 * TmTk uses a very simple implementation of linked lists. Besides
 * the type TMList, the internal representation of lists is
 * also accessable for efficiency reasons and because revealing
 * the structure makes static creation of lists possible.
 */
typedef struct TMList *TMList; 


/** \struct TMList
 * The internal representation of lists.
 * Here is the idiom for looping over lists in TmTk:
 * \verbatim
  
   TMList lp,list = NULL; 
   list = tm_list_push(list,some_item1);
   list = tm_list_push(list,some_item2);
   list = tm_list_push(list,some_item3);
   
   for(lp=list;lp;lp=lp->next)
   {
   	void *v = lp->content;
        ...do something with v
   }
   \endverbatim
 * You will see this code everywhere in TmTk.
 */
struct TMList {
    TMList next;
    void *content;
}; 


/** Delete a list.
 * This will not delete the items. This is the responsibility
 * of the user.
 * \param pself pointer to the list (will be set to NULL)
 */
TM_API(void) tm_list_delete(TMList *pself);



/** Add an item to the front of the list.
 * This is equally efficient for short and long lists. If order
 * is an issue and the new item needs to be ad the end, use
 * tm_list_append() instead.
 * \warning { It is a common mistake to path an uninitialized list when
 * this is called the first time on a variable of type TMList. Allways set your
 * lists to NULL initially!}
 *
 * \param self the list
 * \param content the item to push
 * \return the list after pushing the new element.
 */
TM_API(TMList) tm_list_push(TMList self,void *content);


/** Add an item to the end of the list.
 * The whole list will be traversed until the end, so this
 * becomes inefficient for long lists. Consider using
 * tm_list_push() if order is not an issue.
 * \param self the list
 * \return the list if self isn't NULL, this is equal to self
 */
TM_API(TMList) tm_list_append(TMList self,void *content);


/** Remove an item from the fron of the list.
 * Removes the item and returns the list (which will be different
 * after the removal). 
 * \param self the list
 * \param x if non-NULL, x will be set to point to the popped item
 * \return the list
 */
TM_API(TMList) tm_list_pop(TMList self, void **x);

/** Get the number of items in a list.
 * Note that this traverses the whole list and thus
 * becomes inefficient for long lists.
 * \param self the list
 * \return the number of items in the list
 */
TM_API(int) tm_list_size(TMList list);


/** Remove item from a list.
 *
 */
TM_API(TMList) tm_list_remove(TMList list, void *v, int cmp(const void *x,const void *y));


/** Copy a list.
 * Makes a copy of a list (items are not copied).
 */

TM_API(TMList) tm_list_copy(TMList list);

/** @} */
#if 0
OV_API(void) list_delete(GW gw,List *list);
OV_API(List) list_list(GW gw,void *,...);
OV_API(int) list_intersectionEmpty(GW gw,List l1,List l2, int cmp(const void *x,const void *y));
OV_API(List) list_concat(GW gw,List l1,List l2);
/* OV_API(int) list_equals(GW gw,List,List,int (*equal)(const void *,const void *));*/
OV_API(List) list_copy(GW gw,List list);
OV_API(int) list_contains(GW gw,List, const void *,int cmp(const void *x,const void *y));
OV_API(long) list_size(GW gw,List);
OV_API(List) list_pop(GW gw,List list, void **x);
OV_API(void) list_apply(GW gw,List list, void apply(void **x, void *cl), void *cl);
#endif

TM_API(void) TMList_delete(TMList *pself);
TM_API(TMList) TMList_push(TMList self,void *content);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

