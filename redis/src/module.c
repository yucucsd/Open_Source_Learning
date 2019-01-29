/* Private data structures used by the modules system. Those are data
structures that are never exposed to Redis Modules, if not as void
pointers that have an API the module can call with them. */

/* This structure represents a module inside the system. */
struct RedisModule
{
	void* handle;	/* Module dlopen() handle. */
	char* name;  	/* Module name. */
	int ver;	/* Module version. We use just progressive integers. */
	int apiver;	/* Module API version as requested during initialization. */
	list *types;	/* Module data types. */
};
typedef struct RedisModule RedisModule;

/*This structure represents the context in which Redis modules operate.
Most APIs module can access, get a pointer to the context, so that the API
implementation can hold state across calls, or remember what to free after
the call and so forth.

Note that not all the context structure is always filled with actual values
but only the fields needed in a given context. */

struct RedisModuleCtx
{
	void *getapifuncptr;		/* NOTE: Must be the first field. */
	struct RedisModule *module;     /* Module reference. */
	client *client;			/* Client calling a command. */
	struct RedisModuleBlockedClient *block_client; /* Blocked client for
							thread safe context. */
	struct AutoMemEntry *amqueue;   /* Auto memory queue of objects to free. */
	int amqueue_len; 		/* Number of slots in amqueue*/
	int amqueue_used; 		/* Number of used slots in amqueue. */
	int flags; 			/* REDISMODULE_CTX... flags. */
	void **postponed_arrays; 	/* To set with RM_ReplySetArrayLength(). */
	int postponed_arrays_count;	/* Number of entries in postponed_arrays. */
	void *blocked_privdata;		/* Privdata set when unblocking a client. */
	/* Used if there is the REDISMODULE_CTX_KEYS_POS_REQUEST flag set. */
	int *keys_pos;
	int key_count;

	struct RedisModulePoolAllocBlock *pa_head;
}

#define REDISMODULE_CTX_INIT {(void*)(unsigned long) &RM_GetApi, NULL, NULL, NULL, NULL, 0, 0, 0, NULL, 0, NULL, NULL, 0, NULL}
/* Load all the modules in the server.loadmodule_queue list, which is
populated by 'loadmodule' directives in the configuration file.
We can't load modules directly when processing the configuration file
because the server must be fully initialized before loading modules.

The function aborts the server on errors, since to start with missing
modules is not considered sane: clients may rely on the existence of 
given commands, loading AOF also may need some modules to exist, and
if this instance is a slave, it must understand commands from master.
*/

void moduleLoadFromQueue(void)
{
	listIter li;
	listNode *ln;

	listRewind(server.loadmodule_queue, &li);
	while((ln = listNext(&li)))
	{
		struct moduleLoadqueueEntry *loadmod = ln->value;
		if (moduleLoad(loadmod->path, (void **)loadmod->argv, loadmod->argc) == C_ERR)
		{
			serverLog(LL_WARNING, "Can't load module from %s: server aborting", loadmod->path);
			exit(1);
		}
	}
}


/* Load a module and initialize it. On success C_OK i s returned, otherwise
C_ERR is returned.*/
int moduleLoad(const char *path, void **module_argv, int module_argc)
{
	int (*onload)(void*, void**, int);
	void *handle;
	RedisModuleCtx ctx = REDISMODULE_CTX_INIT;

	handle = dlopen(path, RTLD_NOW|RTLD_LOCAL);
	if (handle == NULL)
	{
		serverLog(LL_WARNING, "Module %s failed to load: %s", path, dlerror());
		exit(1);
	}

	onload = (int (*)(void*, void**, int))(unsigned long) dlsym(handle, "RedisModule_OnLoad");
	if (onload == NULL)
	{
		dlclose(handle);
		serverLog(LL_WARNING, "Module %s does not export RedisModule_OnLoad() "
			"symbol. Module not loaded.", path);
		return C_ERR;
	}
	if (onload((void*)&ctx, module_argv, module_argc) == REDISMODULE_ERR)
	{
		if (ctx.module)
		{
			moduleUnregisterCommands(ctx.module);
			moduleFreeModuleStructure(ctx.module);
		}
		dlclose(handle);
		serverLog(LL_WARNING, "Module %s initialization failed. Module not loaded", path);
		return C_ERR;
	}

	/* Redis module loaded! Register it. */
	dictAdd(modules, ctx.module->name. ctx.module);
	ctx.module->handle = handle;
	serverLog(LL_NOTICE, "Module '%s' loaded from %s". ctx.module->name, path);
	moduleFreeContext(&ctx);
	return C_OK;
}
