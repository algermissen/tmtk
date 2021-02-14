#include "tmtable.h"
#include "limits.h"
#include "tmmem.h"
#include "tmassert.h"

struct TMTable {
	int size;
	int (*cmp)(const void *x, const void *y);
	unsigned (*hash)(const void *key);
	int length;
	unsigned timestamp;
	struct binding {
		struct binding *link;
		const void *key;
		void *value;
	} **buckets;
};

static int cmpatom(const void *x,const void *y)
{
	return x != y;
}

static unsigned hashatom(const void *key)
{
	return (unsigned long)key >> 2;
}


TMTable tm_table_new(int hint, int cmp(const void *x, const void *y),
		unsigned hash(const void *key))
{
	TMTable table;
	int i;
	static int primes[] = { 509,509,1021,2053,4093,8191,16381,32771,
				65521, INT_MAX};

	assert(hint >= 0);
	for(i = 1; primes[i] < hint; i++)
		;

	table = (TMTable)TM_ALLOC(sizeof(*table) + primes[i-1]*sizeof(table->buckets[0]));
	table->size = primes[i-1];
	table->cmp = cmp ? cmp : cmpatom;
	table->hash = hash ? hash : hashatom;
	table->buckets = (struct binding **)(table + 1);
	for(i = 0; i < table->size; i++)
		table->buckets[i] = NULL;
	table->length = 0;
	table->timestamp = 0;

	return table;
} 


void tm_table_delete(TMTable *pself)
{
	assert(pself && *pself);

	/* TBD free buckets */

	TM_FREE(*pself);
	*pself = NULL;
}

void *tm_table_get(TMTable table, const void *key)
{
	int i;
	struct binding *p;

	assert(table);
	assert(key);

	i = (*table->hash)(key)%table->size;
	for(p = table->buckets[i]; p; p = p->link) 
		if((*table->cmp)(key,p->key) == 0)
			break;

	return p ? p->value : NULL;
}


void *tm_table_put(TMTable table, const void *key, void *value)
{
	int i;
	struct binding *p;
	void *prev;

	assert(table);
	assert(key);

	i = (*table->hash)(key)%table->size;
	for(p = table->buckets[i]; p; p = p->link)
		if((*table->cmp)(key,p->key) == 0)
			break;

	if(p == NULL)
	{
		p = (struct binding *)malloc(sizeof(struct binding));
		assert(p);
		p->key = key;
		p->link = table->buckets[i];
		table->buckets[i] = p;
		table->length++;
		prev = NULL;
	} else {
		prev = p->value;
	}

	p->value = value;
	table->timestamp++;

	return prev;

}

void tm_table_apply(TMTable table, void apply(const void *key,void **value, void *cl), void *cl)
{
	int i;
	unsigned stamp;
	struct binding *p;

	assert(table);
	assert(apply);

	stamp = 0;
	/* stamp = table->timestamp; */
	for(i = 0; i < table->size; i++)
	{
		for(p = table->buckets[i]; p; p = p->link)
		{
			apply(p->key,&p->value, cl);
			/* assert(table->timestamp == stamp); */
		}
	}
}

TMList tm_table_keys(TMTable table, TMList *listp)
{
	int i;
	struct binding *p;
	TMList lp;

	assert(table);
	assert(listp);
	lp = NULL;
	for(i = 0; i < table->size; i++)
	{
		for(p = table->buckets[i]; p; p = p->link)
		{
			lp = tm_list_push(lp,(void*)p->key);
		}
	}
	*listp = lp;
	return lp;

}

void *tm_table_remove(TMTable table, const void *key)
{

	int i;
	struct binding **pp;

	assert(table);
	assert(key);

	/* table->timestamp++; */
	i = (*table->hash)(key)%table->size;
	for(pp = &table->buckets[i]; *pp; pp = &(*pp)->link)
	{
		if((*table->cmp)(key,(*pp)->key) == 0)
		{
			struct binding *p = *pp;
			void *value = p->value;
			*pp = p->link;
			TM_FREE(p);
			p = NULL;
			table->length--;
			return value;
		}

	}
	return NULL;	


}
