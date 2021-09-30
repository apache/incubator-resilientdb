#include "global.h"
#include "mem_alloc.h"
#include "time.h"

uint64_t get_part_id(void *addr)
{
	return ((uint64_t)addr / PAGE_SIZE) % g_part_cnt;
}

uint64_t key_to_part(uint64_t key)
{
	if (g_part_alloc)
		return key % g_part_cnt;
	else
		return 0;
}

uint64_t merge_idx_key(UInt64 key_cnt, UInt64 *keys)
{
	UInt64 len = 64 / key_cnt;
	UInt64 key = 0;
	for (UInt32 i = 0; i < len; i++)
	{
		assert(keys[i] < (1UL << len));
		key = (key << len) | keys[i];
	}
	return key;
}

uint64_t merge_idx_key(uint64_t key1, uint64_t key2)
{
	assert(key1 < (1UL << 32) && key2 < (1UL << 32));
	return key1 << 32 | key2;
}

uint64_t merge_idx_key(uint64_t key1, uint64_t key2, uint64_t key3)
{
	assert(key1 < (1 << 21) && key2 < (1 << 21) && key3 < (1 << 21));
	return key1 << 42 | key2 << 21 | key3;
}

void init_client_globals()
{
	g_servers_per_client = g_node_cnt;
	if (g_node_cnt > g_client_node_cnt)
	{
		g_clients_per_server = 1;
	}
	else
	{
		g_clients_per_server = g_client_node_cnt / g_node_cnt;
	}
	g_server_start_node = 0;
	printf("Node %u: servicing %u total nodes starting with node %u\n", g_node_id, g_servers_per_client, g_server_start_node);
}

/****************************************************/
// Global Clock!
/****************************************************/

uint64_t get_wall_clock()
{
	timespec *tp = new timespec;
	clock_gettime(CLOCK_REALTIME, tp);
	uint64_t ret = tp->tv_sec * 1000000000 + tp->tv_nsec;
	delete tp;
	return ret;
}

uint64_t get_server_clock()
{
#if defined(__i386__)
	uint64_t ret;
	__asm__ __volatile__("rdtsc"
						 : "=A"(ret));
#elif defined(__x86_64__)
	unsigned hi, lo;
	__asm__ __volatile__("rdtsc"
						 : "=a"(lo), "=d"(hi));
	uint64_t ret = ((uint64_t)lo) | (((uint64_t)hi) << 32);
	ret = (uint64_t)((double)ret / CPU_FREQ);
#else
	timespec *tp = new timespec;
	clock_gettime(CLOCK_REALTIME, tp);
	uint64_t ret = tp->tv_sec * 1000000000 + tp->tv_nsec;
	delete tp;
#endif
	return ret;
}

uint64_t get_sys_clock()
{
	if (TIME_ENABLE)
		return get_server_clock();
	return 0;
}

std::string hexStr(const char *data, int len)
{
	std::string s(len * 2, ' ');
	for (int i = 0; i < len; ++i)
	{
		s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
		s[2 * i + 1] = hexmap[data[i] & 0x0F];
	}
	return s;
}

void myrand::init(uint64_t seed)
{
	this->seed = seed;
}

uint64_t myrand::next()
{
	seed = (seed * 1103515247UL + 12345UL) % (1UL << 63);
	return (seed / 65537) % RAND_MAX;
}

uint64_t get_commit_message_txn_id(uint64_t txn_id)
{
	uint64_t result = txn_id / get_batch_size();
	result = (result + 1) * get_batch_size();
	return result - 1;
}
uint64_t get_prep_message_txn_id(uint64_t txn_id)
{
	uint64_t result = txn_id / get_batch_size();
	result = (result + 1) * get_batch_size();
	return result - 1;
}
uint64_t get_checkpoint_message_txn_id(uint64_t txn_id)
{
	uint64_t result = txn_id / get_batch_size();
	result = (result + 1) * get_batch_size();
	return result - 6;
}
uint64_t get_execute_message_txn_id(uint64_t txn_id)
{
	uint64_t result = txn_id / get_batch_size();
	result = (result + 1) * get_batch_size();
	return result - 2;
}