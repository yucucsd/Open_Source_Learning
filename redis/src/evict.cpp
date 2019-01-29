class evictionPoolEntry
{
	unsigned long long idle; /* Object idle time (inverse frequency for LFU) */
	string key;		 /* Key name. */
	string cached;		 /* Cached object for key name. */
	int dbid;		 /* Key DB number */
};

vector<evictionPoolEntry> EvictionPoolLRU;
/* LRU approximation algorithm

Redis uses an approximation of the LRU algorithm that runs in constant
memory. Every time there is a key to expire, we sample N keys (with N
very small, usually in around 5) to populate a pool of best keys to evict
of M keys (the pool size is defined by EVPOOL_SIZE).

The N keys sampled are added in the pool of good keys to expire (the one
with an old access time) if they are better than one of the current keys
in the pool.

After the pool is populated, the best key we have in the pool is expired.
However, note that we don't remove keys from the pool when they are deleted,
so the pool may contain keys that are no longer exist.

When we try to evict a key, and all the entries in the pool don't exist,
we populate it again. This time we'll be sure that the pool has at least
one key that can be evicted, if there is at least one key that can be evicted
in the whole database. */

/* Create a new eviction pool. */
void evictionPoolAlloc(void)
{
	vector<evictionPoolEntry> ep;

	for (int j = 0; j < EVPOOL_SIZE; ++j)
	{
		evictionPoolEntry t;
		t.idle = 0;
		t.dbid = 0;
		ep.push_back(t);
	}
	EvictionPoolLRU.assign(ep.begin(), ep.end());
}
