/*
 * $Id$
 *
 * Copyright (c) 2002 Jan Algermissen
 * See the file "COPYING" for copying permission.
 *
 */

#include <tmlist.h>
#include <tmtrace.h>
#include <tmmem.h>
#include <tmassert.h>

int tm_list_size(TMList list)
{
	long n = 0;
	for(;list;list=list->next) n++;
	return n;
}

TMList tm_list_push(TMList self, void *content)
{
	TMList newItem;
	TM_NEW(newItem);
	newItem->content = content;
	newItem->next = self;
	return newItem;
}

TMList tm_list_append(TMList self, void *content)
{
	TMList lp,newItem;
	TM_NEW(newItem);

	newItem->content = content;
	newItem->next = NULL;
	if(self == NULL)
	{
		self = newItem;
		return self;
	}
	for(lp = self; lp->next != NULL; lp=lp->next)
		;
	lp->next = newItem;
	return self;
}
TMList tm_list_pop(TMList self, void **x)
{
	TMTRACE(TM_GRAPH_TRACE,"x1\n");
	if(self)
	{
		TMList head = self->next;
		if(x)
			*x = self->content;
		TM_FREE(self);
		return head;
	}
	else
	{
		if(x)
			*x = NULL;
		return NULL;
	}
}

void tm_list_delete(TMList *pself)
{
	TMList next;
	assert(pself);
	for( ; *pself; *pself = next)
	{
		next = (*pself)->next;
		TM_FREE(*pself);
	}
}

TMList tm_list_copy(TMList list)
{
	TMList n = NULL;
	if(!list)
		return NULL;
	for( ; list; list = list->next)
		n = tm_list_push(n,list->content);

	return n;
}

TMList tm_list_remove(TMList list, void *v, int cmp(const void *x,const void *y))
{
	TMList prev,lp;
	assert(cmp == NULL);
	if(list == NULL)
		return(NULL);
	prev = NULL;
	for(lp=list;lp;lp=lp->next)
	{
		TMList tmp = NULL;
		if( !((cmp!=NULL) && (cmp(lp->content,v)==0) )
		&&  !((cmp==NULL) && (lp->content == v))   )
		{
			prev = lp;
			continue;
		}

		if(!prev)
		{
			tmp = list;
			list = list->next;
			TM_FREE(tmp);
		}
		else
		{
			assert(prev->next == lp);
			prev->next = lp->next;
			TM_FREE(lp);
		}
		break;

	}
#if 0
	prev = NULL;
	for(lp=list;lp;lp=lp->next)
	{
		TMList tmp = NULL;
		if(lp->content != v )
		{
			prev = lp;
			continue;
		}

		if(!prev)
		{
			tmp = list;
			list = list->next;
			TM_FREE(tmp);
		}
		else
		{
			assert(prev->next == lp);
			prev->next = lp->next;
			TM_FREE(lp);
		}
		break;

	}
#endif
	return list;
}


#if 0

/**
 * Create a new list from the items in the variable argument list.
 * Note that the arguments have to be casted to (void *) 
 * <pre>
 * list = GWList_list(gw,"elem1","elem2","elem3"
 * </pre>
 *
 * @param gw the GW handle
 * @param x,... the items to build the list from
 * @return the new list
 */

GWList_T GWList_list(GW gw,void *x,...)
{
	GWList_T list, *p = &list;
	va_list ap;

	va_start(ap,x);

	for( ; x; x = va_arg(ap,void*))
	{
		GW_NEW(gw,*p);
		(*p)->content = x;
		p = &(*p)->next;
	}
	*p = NULL;
	va_end(ap);
	return list;

}

/** Make a copy of a list.
 * The content is not copied.
 *
 * @param list the list to be copied
 * @return the copy
 */
GWList_T GWList_copy(GW gw,GWList_T list)
{
	GWList_T ret = NULL;
	if(!list)
		return NULL;
	for( ; list; list = list->next)
		ret = GWList_push(gw,ret,list->content);

	return ret;
}
/** Check if list contains v.
 * Equality is checked trough
 * pointer euality
 * @param list the list
 * @param v pointer to content
 * @return Return 1 if v is in list else 0
 */
int GWList_contains(GW gw,GWList_T list, const void *v,int cmp(const void *x,const void *y))
{
	
	if(!list)
		return 0;	
	
	for( ; list ; list = list->next)
		if(cmp && cmp(list->content,v) == 0 )
			return 1;
		else if(list->content == v)
			return 1;

	return 0;

}
/** Checks two lists for equality.
 * GWList items are compared by pointer equality.
 * @param l1 list1
 * @param l2 list2
 * @return 1 if lists are equal, 0 if they are not
 */
/*
int GWList_equals(GW gw,GWList_T l1, GWList_T l2)
{
	GWList_T p1,p2;

	for(p1 = l1; p1; p1 = p1->next)
	{
		int found;
		found = 0;
		for(p2 = l2; p2; p2 = p2->next)
		{
			if(p1->content == p2->content)
			{
				found = 1;	
				break;
			}
		}
		if(!found)
			return 0;
	}
	for(p2 = l2; p2; p2 = p2->next)
	{
		int found;
		found = 0;
		for(p1 = l1; p1; p1 = p1->next)
		{
			if(p1->content == p2->content)
			{
				found = 1;	
				break;
			}
		}
		if(!found)
			return 0;
	}

	return 1;
}
*/
/** Check if the intersection of l1 nd l2 is the empty set.
 * @param l1 list1
 * @param l2 list2
 * @param cmp compare function
 * @return 1 if intersection 
 */
int GWList_intersectionEmpty(GW gw,GWList_T l1,GWList_T l2,int cmp(const void *x,const void *y))
{
	GWList_T p1,p2;

	for(p1 = l1; p1; p1 = p1->next)
	{
		for(p2 = l2; p2; p2 = p2->next)
		{
			if(cmp(p1->content,p2->content) == 0)
			{
				return 0;
			}
		}
	}
	for(p2 = l2; p2; p2 = p2->next)
	{
		for(p1 = l1; p1; p1 = p1->next)
		{
			if(cmp(p1->content,p2->content) == 0)
			{
				return 0;
			}
		}
	}

	return 1;

}
/** Concatenate two lists.
 * This will add all the elements of l2 to l1,
 * returning the resulting list.
 * l2 isn't changed. 
 * @param l1 the list to concat to
 * @param l2 the concated list
 * @return l1 (as in push) 
 */

GWList_T GWList_concat(GW gw,GWList_T l1,GWList_T l2)
{
	for(;l2;l2=l2->next)
	{
		l1 = GWList_push(gw,l1,l2->content);
	}
	return l1;

}
/** Apply a function
 *
 * @param gw the GW handle
 * @param list the list
 * @param apply pointer to a function that is to be applied to
 * each list element. 
 * 
 */
void GWList_apply(GW gw,GWList_T list, void apply(void **x, void *cl), void *cl)
{
	assert(apply);
	for(;list;list=list->next)
		apply(&list->content,cl);
}


#endif

/* depricated (FIXME) */
TMList TMList_push(TMList self, void *content)
{
	return tm_list_push(self,content);
}

void TMList_delete(TMList *pself)
{
	tm_list_delete(pself);
}
