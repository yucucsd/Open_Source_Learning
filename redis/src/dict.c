/* ------------------ hash function ---------------------- */
static uint8_t dict_hash_function_seed[16];
void dictSetHashFunctionSeed(uint8_t *seed)
{
	memcpy(dict_hash_function_seed, seed, sizeof(dict_hash_function_seed));
}
static unsigned int dictGenHashFunction(const void *key,  int len)
{
	return siphash(key, len, dict_hash_function_seed);
}

/* Create a new hash table */
dict* dictCreate(dictType &type, void *privDataPtr)
{
	dict *d = new dict();
	d.init()
}

int dictAdd(dict *d, void* key, void* val)
{
	dictEntry *entry = dictAddRaw(d, key, NULL);

	if (!entry)
		return DICT_ERR;
	dictSetVal(d, entry, val);
	return DICT_OK;
}

/* Low lover add or find:
This function adds the entry but instead of setting a value returns the
dictEntry structure to the user, that will make sure to fill the value 
field as he wishes.

This function is also directly exposed to the user API to be called 
mainly in order to store non-pointers inside the hash value, example:

entry = dictAddRaw(dict, mykey, NULL);
if (entry != NULL) dictSetSignedIntegerVal(entry, 1000);

Return values:

If key already exists NULL is returned and "existing" is populated with
the existing entry if existing is not NULL.

If key was added, the hash entry is returned to be manipulated by the caller.
*/
dictEntry* dictAddRaw(dict *d, void *key, dictEntry **existing)
{
	long index;
	dictEntry *entry;
	dictht *ht;

	if (dictIsRehashing(d))
		_dictRehashStep(d);

	/* Get the index of the new element, or -1 if already exists. */
	if ((index = _dictKeyIndex(d, key, dictHashKey(d, key), existing)) == -1)
		return NULL;

	/* Allocate memory and store the new entry. 
	Insert the element in the top, with the assumption that in a database
	system it is more likely that recently added entries are accessed more
	frequently. */
	ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
	entry = zmalloc(sizeof(*entry));
	entry->next = ht->table[index];
	ht->table[index] = entry;
	ht->used++;

	/* Set the sh entry fields. */
	dictSetKey(d, entry, key);
	return entry;
}

/* Returns the index of a free slot that can be populated with a hash entry 
for the given 'key'. If the key already exists, -1 is returned and the optional
output parameter may be filled.

Note that if we are in the process of rehashing the hash table, the index is always
returned in the context of the second (new) hash table. */


/* static function could not be invoked in another file. */
static long _dictKeyIndex(dict *d, const void* key, uint64_t hash, dictEntry** existing)
{
	unsigned long idx, table;
	dictEntry *he;
	if (existing)
		*existing = NULL;

	/* Expand the table if needed. */
	if (_dictExpandIfNeeded(d) == DICT_ERR)
		return -1;
	for (table = 0; table <= 1; table++)
	{
		idx = hash & d->ht[table].sizemask;
		/* Search if this slot does not already contain the given key. */
		he = d->ht[table].table[idx];
		while(he)
		{
			if (key == he->key || dictCompareKeys(d, key, he->key))
			{
				if (existing)
					*existing = he;
				return -1;
			}
			he = he->next;
		}
		if (!dictIsRehashing(d))
			break;
	}
	return idx;
}
