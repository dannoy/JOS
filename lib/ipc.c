// User-level IPC library routines

#include <inc/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
//
// Hint:
//   Use 'thisenv' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value, since that's
//   a perfectly valid place to map a page.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
    //panic("ipc_recv not implemented");
    /* lj */
    int ret = 0;
    void *addr = NULL;

    #define SET_VALUE(var, value) \
            do { \
                if(var) { \
                    *var = value; \
                }\
            }while (0) \

    addr = pg ? pg : (void *)-1;

    if((ret = sys_ipc_recv(addr)) < 0) {
        //SET_VALUE(from_env_store , 0);
        SET_VALUE(perm_store ,0);
    }
    else if(0 == ret) {
        //SET_VALUE(from_env_store , thisenv->env_ipc_from);
        SET_VALUE(perm_store , thisenv->env_ipc_perm);
        ret = thisenv->env_ipc_value;
    }
    SET_VALUE(from_env_store , thisenv->env_ipc_from);

    #undef SET_VALUE
    //cprintf("%x get from %x %d %d\n", thisenv->env_id, thisenv->env_ipc_from, thisenv->env_ipc_value, ret);
    
	return ret;
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
	// LAB 4: Your code here.
    //panic("ipc_send not implemented");
    /* lj */
    int ret = 0;
    void *addr = NULL;
    addr = pg ? pg : (void *)-1;
    //cprintf("%x->%x [%d]\n", thisenv->env_id, to_env, val);
    
    while (1) {
        if(-E_IPC_NOT_RECV == ret) {
            sys_yield();
        }

        ret = sys_ipc_try_send(to_env, val, addr, perm);

        if(0 == ret) {
            break;
        }
        else if(-E_IPC_NOT_RECV == ret) {
            //cprintf("%x continue\n", thisenv->env_id);
            continue;
        }
        else if(ret < 0) {
            panic("ipc_send error:%e", ret);
        }
    }
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;
	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;
	return 0;
}
