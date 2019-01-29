#include <vector>

typedef struct dictEntry
{
	void* key;
	union { /* share memory*/
		void* val;
		uint64_t u64;
		int64_t s64;
		double d;
	} v;
	struct dictEntry *next;
} dictEntry;

class dictType
{
	/* function pointers */
	uint64_t (*hashFunction)(const void* key);
	void* (*keyDup)()
};

/* This is our hash table structure. Every dictionary has two of this
as we implement incremental rehashing, for the old to the new table. */
typedef struct dictht 
{
	dictEntry** table;
	unsigned long size;
	unsigned long sizemask;
	unsigned long used;
} dictht;

typedef struct dict
{
	dictType *type;
	void* privdata;
	dictht ht[2];
	long rehashidx; /* rehashing not in progress if rehashidx == -1 */
	unsigned long iterators; /* number of iterators currently running */
} dict;

/* -------------------------------- Macros -------------------------------- */

#define dictSetKey(d, entry, _key_) do { \
	if ((d)->type->keyDup) \
		(entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
	else \
		(entry)->key = (_key_); \
} while(0)

#define dictHashKey(d, key) (d)->type->hashFunction(key)
