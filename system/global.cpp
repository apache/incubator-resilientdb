#include "global.h"
#include "mem_alloc.h"
#include "stats.h"
#include "sim_manager.h"
#include "query.h"
#include "client_query.h"
#include "transport.h"
#include "work_queue.h"

#include "msg_queue.h"
#include "pool.h"
#include "txn_table.h"
#include "client_txn.h"
#include "txn.h"
#include "../config.h"

mem_alloc mem_allocator;
Stats stats;
SimManager *simulation;
Query_queue query_queue;
Client_query_queue client_query_queue;
Transport tport_man;
TxnManPool txn_man_pool;
TxnPool txn_pool;
TxnTablePool txn_table_pool;
QryPool qry_pool;
TxnTable txn_table;
QWorkQueue work_queue;

MessageQueue msg_queue;
Client_txn client_man;

bool volatile warmup_done = false;
bool volatile enable_thread_mem_pool = false;
pthread_barrier_t warmup_bar;

bool g_key_order = KEY_ORDER;
bool g_ts_batch_alloc = TS_BATCH_ALLOC;
UInt32 g_ts_batch_num = TS_BATCH_NUM;
int32_t g_inflight_max = MAX_TXN_IN_FLIGHT;
uint64_t g_msg_size = MSG_SIZE_MAX;
int32_t g_load_per_server = LOAD_PER_SERVER;

bool g_hw_migrate = HW_MIGRATE;

volatile UInt64 g_row_id = 0;
bool g_part_alloc = PART_ALLOC;
bool g_mem_pad = MEM_PAD;
ts_t g_query_intvl = QUERY_INTVL;
UInt32 g_part_per_txn = PART_PER_TXN;
double g_perc_multi_part = PERC_MULTI_PART;
double g_txn_read_perc = 1.0 - TXN_WRITE_PERC;
double g_txn_write_perc = TXN_WRITE_PERC;
double g_tup_read_perc = 1.0 - TUP_WRITE_PERC;
double g_tup_write_perc = TUP_WRITE_PERC;
double g_zipf_theta = ZIPF_THETA;
double g_data_perc = DATA_PERC;
double g_access_perc = ACCESS_PERC;
bool g_prt_lat_distr = PRT_LAT_DISTR;
UInt32 g_node_id = 0;
UInt32 g_node_cnt = NODE_CNT;
UInt32 g_part_cnt = PART_CNT;
UInt32 g_virtual_part_cnt = VIRTUAL_PART_CNT;
UInt32 g_core_cnt = CORE_CNT;
UInt32 g_thread_cnt = THREAD_CNT;
UInt32 g_worker_thread_cnt = WORKER_THREAD_CNT;
UInt32 g_batching_thread_cnt = BATCH_THREAD_CNT;
UInt32 g_checkpointing_thread_cnt = CHECKPOINT_THREAD_CNT;
UInt32 g_execution_thread_cnt = EXECUTE_THREAD_CNT;

UInt32 g_rem_thread_cnt = REM_THREAD_CNT;
UInt32 g_send_thread_cnt = SEND_THREAD_CNT;
UInt32 g_total_thread_cnt = g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt;
UInt32 g_total_client_thread_cnt = g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt;
UInt32 g_total_node_cnt = g_node_cnt + g_client_node_cnt + g_repl_cnt * g_node_cnt;
UInt64 g_synth_table_size = SYNTH_TABLE_SIZE;
UInt32 g_req_per_query = REQ_PER_QUERY;
bool g_strict_ppt = STRICT_PPT == 1;
UInt32 g_field_per_tuple = FIELD_PER_TUPLE;
UInt32 g_init_parallelism = INIT_PARALLELISM;

// Client Related Data.
UInt32 g_client_node_cnt = CLIENT_NODE_CNT;
UInt32 g_client_thread_cnt = CLIENT_THREAD_CNT;
UInt32 g_client_rem_thread_cnt = CLIENT_REM_THREAD_CNT;
UInt32 g_client_send_thread_cnt = CLIENT_SEND_THREAD_CNT;
UInt32 g_servers_per_client = 0;
UInt32 g_clients_per_server = 0;
uint64_t last_valid_txn = 0;
uint64_t get_last_valid_txn()
{
	return last_valid_txn;
}

void set_last_valid_txn(uint64_t txn_id)
{
	last_valid_txn = txn_id;
}

UInt32 g_server_start_node = 0;
UInt32 g_this_thread_cnt = ISCLIENT ? g_client_thread_cnt : g_thread_cnt;
UInt32 g_this_rem_thread_cnt = ISCLIENT ? g_client_rem_thread_cnt : g_rem_thread_cnt;
UInt32 g_this_send_thread_cnt = ISCLIENT ? g_client_send_thread_cnt : g_send_thread_cnt;
UInt32 g_this_total_thread_cnt = ISCLIENT ? g_total_client_thread_cnt : g_total_thread_cnt;

UInt32 g_max_txn_per_part = MAX_TXN_PER_PART;
UInt32 g_network_delay = NETWORK_DELAY;
UInt64 g_done_timer = DONE_TIMER;
UInt64 g_seq_batch_time_limit = SEQ_BATCH_TIMER;
UInt64 g_prog_timer = PROG_TIMER;
UInt64 g_warmup_timer = WARMUP_TIMER;
UInt64 g_msg_time_limit = MSG_TIME_LIMIT;

double g_mpr = MPR;
double g_mpitem = MPIR;

UInt32 g_repl_type = REPL_TYPE;
UInt32 g_repl_cnt = REPLICA_CNT;

/******** Key storage for signing. ***********/
//ED25519 and RSA
string g_priv_key;							   //stores this node's private key
string g_public_key;						   //stores this node's public key
string g_pub_keys[NODE_CNT + CLIENT_NODE_CNT]; //stores public keys of other nodes.
//CMAC
string cmacPrivateKeys[NODE_CNT + CLIENT_NODE_CNT];
string cmacOthersKeys[NODE_CNT + CLIENT_NODE_CNT];
//ED25519
CryptoPP::ed25519::Verifier verifier[NODE_CNT + CLIENT_NODE_CNT];
CryptoPP::ed25519::Signer signer;

// Receiving keys
uint64_t receivedKeys[NODE_CNT + CLIENT_NODE_CNT];

/*********************************************/

// Entities for ensuring successful key exchange.
std::mutex keyMTX;
bool keyAvail = false;
uint64_t totKey = 0;

uint64_t indexSize = 2 * g_client_node_cnt * g_inflight_max;
#if GBFT
uint64_t g_min_invalid_nodes = (gbft_cluster_size - 1) / 3; //min number of valid nodes
#elif SGX

#if A2M || MIN_PBFT || MIN_ZYZ
uint64_t g_min_invalid_nodes = (g_node_cnt - 1) / 2;
#else
uint64_t g_min_invalid_nodes = (g_node_cnt - 1) / 3;
#endif

#else
uint64_t g_min_invalid_nodes = (g_node_cnt - 1) / 3; //min number of valid nodes
#endif

// Funtion to calculate hash of a string.
string calculateHash(string str)
{
	byte const *pData = (byte *)str.data();
	unsigned int nDataLen = str.size();
	byte aDigest[CryptoPP::SHA256::DIGESTSIZE];

	CryptoPP::SHA256().CalculateDigest(aDigest, pData, nDataLen);
	return std::string((char *)aDigest, CryptoPP::SHA256::DIGESTSIZE);
}

// Entities for maintaining g_next_index.
uint64_t g_next_index = 0; //index of the next txn to be executed
std::mutex gnextMTX;
void inc_next_index()
{
	gnextMTX.lock();
	g_next_index++;
	gnextMTX.unlock();
}

uint64_t curr_next_index()
{
	uint64_t cval;
	gnextMTX.lock();
	cval = g_next_index;
	gnextMTX.unlock();
	return cval;
}

// Entities for handling checkpoints.
uint32_t g_last_stable_chkpt = 0; //index of the last stable checkpoint
uint32_t g_txn_per_chkpt = TXN_PER_CHKPT;
uint64_t last_deleted_txn_man = 0;

uint64_t txn_per_chkpt()
{
	return g_txn_per_chkpt;
}

void set_curr_chkpt(uint64_t txn_id)
{
	if (txn_id > g_last_stable_chkpt)
		g_last_stable_chkpt = txn_id;
}

uint64_t get_curr_chkpt()
{
	return g_last_stable_chkpt;
}

uint64_t get_last_deleted_txn()
{
	return last_deleted_txn_man;
}

void inc_last_deleted_txn()
{
	last_deleted_txn_man++;
}

// Information about batching threads.
uint64_t expectedExecuteCount = g_batch_size - 2;
uint64_t expectedCheckpoint = TXN_PER_CHKPT - 5;
uint64_t get_expectedExecuteCount()
{
	return expectedExecuteCount;
}

void set_expectedExecuteCount(uint64_t val)
{
	expectedExecuteCount = val;
}

// Variable used by all threads during setup, to mark they are ready
std::mutex batchMTX;
std::mutex next_idx_lock;
uint commonVar = 0;

// Variable used by Input thread at the primary to linearize batches.
uint64_t next_idx = 0;

#if PERSISTENT_COUNTER
uint64_t per_counter_last_response_time = 0;
#endif
#if GBFT
uint64_t get_and_inc_next_idx()
{
	next_idx_lock.lock();
	uint64_t val = next_idx;
	next_idx += gbft_cluster_cnt;
	next_idx_lock.unlock();
	return val;
}
#elif PERSISTENT_COUNTER
bool get_and_inc_next_idx(ClientQueryBatch *clbatch, uint64_t &id)
{
	next_idx_lock.lock();
	uint64_t time = get_sys_clock();
	if (time - per_counter_last_response_time > PERS_COUNTER_RESPONSE_TIME)
	{
		clbatch->tee_signature = "";
		clbatch->tee_signature_size = 0;
		per_counter_last_response_time = time;
		id = next_idx++;
		next_idx_lock.unlock();
		return true;
	}
	else
	{
		next_idx_lock.unlock();
		return false;
	}
}
#elif SGX
uint64_t get_and_inc_next_idx(ClientQueryBatch *clbatch)
{

	struct enclave_message *emsg = send_message_to_enclave(TEE_SEQUENCE_COUNTER);
	uint64_t val = emsg->counter;
	clbatch->tee_signature = (char *)emsg->signature;
	clbatch->tee_signature_size = clbatch->tee_signature.size();

	// printf("The transaction ID recieved is: %ld\n", emsg->counter);
	// fflush(stdout);
	return val;
}
#else
uint64_t get_and_inc_next_idx()
{
	next_idx_lock.lock();
	uint64_t val = next_idx++;
	next_idx_lock.unlock();
	return val;
}
#endif

void set_next_idx(uint64_t val)
{
	next_idx_lock.lock();
	next_idx = val;
	next_idx_lock.unlock();
}

// Counters for input threads to access next socket (only used by replicas).
uint64_t sock_ctr[REM_THREAD_CNT] = {0};
uint64_t get_next_socket(uint64_t tid, uint64_t size)
{
	uint64_t abs_tid = tid % g_this_rem_thread_cnt;
	uint64_t nsock = (sock_ctr[abs_tid] + 1) % size;
	sock_ctr[abs_tid] = nsock;
	return nsock;
}

/** Global Utility functions: **/

// Destination for msgs.
vector<uint64_t> nodes_to_send(uint64_t beg, uint64_t end)
{
	vector<uint64_t> dest;
	for (uint64_t i = beg; i < end; i++)
	{
		if (i == g_node_id)
		{
			continue;
		}
		dest.push_back(i);
	}
	return dest;
}

// STORAGE OF CLIENT DATA
uint64_t ClientDataStore[SYNTH_TABLE_SIZE] = {0};

// returns the current view.
uint64_t get_current_view(uint64_t thd_id)
{
	return get_view(thd_id);
}

#if VIEW_CHANGES == true
// For updating view of different threads.
std::mutex newViewMTX[THREAD_CNT + REM_THREAD_CNT + SEND_THREAD_CNT];
uint64_t newView[THREAD_CNT + REM_THREAD_CNT + SEND_THREAD_CNT] = {0};
uint64_t get_view(uint64_t thd_id)
{
	uint64_t nchange = 0;
	newViewMTX[thd_id].lock();
	nchange = newView[thd_id];
	newViewMTX[thd_id].unlock();
	return nchange;
}

void set_view(uint64_t thd_id, uint64_t val)
{
	newViewMTX[thd_id].lock();
	newView[thd_id] = val;
	newViewMTX[thd_id].unlock();
}
#else
uint64_t get_view(uint64_t thd_id)
{
	return 0;
}
#endif

// Size of the batch
uint64_t g_batch_size = BATCH_SIZE;
uint64_t batchSet[2 * CLIENT_NODE_CNT * MAX_TXN_IN_FLIGHT];
uint64_t get_batch_size()
{
	return g_batch_size;
}
#if !GBFT
uint64_t view_to_primary(uint64_t view, uint64_t node)
{
	return view;
}
#endif

#if GBFT
SpinLockSet<string> gbft_ccm_checklist;
uint32_t last_commited_txn = 0;
UInt32 gbft_cluster_size = GBFT_CLUSTER_SIZE;
UInt32 gbft_cluster_cnt = NODE_CNT / GBFT_CLUSTER_SIZE;
UInt32 gbft_commit_certificate_thread_cnt = GBFT_CCM_THREAD_CNT;

// This variable is mainly used by the client to know its current primary.
uint32_t g_view[NODE_CNT / GBFT_CLUSTER_SIZE] = {0};
std::mutex viewMTX[NODE_CNT / GBFT_CLUSTER_SIZE];
void set_client_view(uint64_t nview, int cluster)
{
	viewMTX[cluster].lock();
	g_view[cluster] = nview;
	viewMTX[cluster].unlock();
}

uint64_t get_client_view(int cluster)
{
	uint64_t val;
	viewMTX[cluster].lock();
	val = g_view[cluster];
	viewMTX[cluster].unlock();
	return val;
}

bool is_primary_node(uint64_t thd_id, uint64_t node)
{
	return view_to_primary(get_current_view(thd_id), node) == node;
}

uint64_t get_cluster_number(uint64_t i)
{
	if (i >= g_node_cnt && i < g_node_cnt + g_client_node_cnt)
	{
		int client_number = i - g_node_cnt;
		return (client_number / (g_client_node_cnt / gbft_cluster_cnt));
	}
	else
		return (i / gbft_cluster_size);
}

uint64_t view_to_primary(uint64_t view, uint64_t node)
{
	return get_cluster_number(node) * gbft_cluster_size + view;
}

int is_in_same_cluster(uint64_t first_id, uint64_t second_id)
{
	return (int)(first_id / gbft_cluster_size) == (int)(second_id / gbft_cluster_size);
}
bool is_local_request(TxnManager *tman)
{
	uint64_t node_id = tman->client_id;
	assert(node_id < g_total_node_cnt);
	int client_number = node_id - g_node_cnt;
	int primary = client_number / (g_client_node_cnt / gbft_cluster_cnt);
	if (is_in_same_cluster(primary * gbft_cluster_size, g_node_id))
	{
		return true;
	}
	return false;
}

#else
// This variable is mainly used by the client to know its current primary.
uint32_t g_view = 0;
std::mutex viewMTX;
void set_client_view(uint64_t nview)
{
	viewMTX.lock();
	g_view = nview;
	viewMTX.unlock();
}

uint64_t get_client_view()
{
	uint64_t val;
	viewMTX.lock();
	val = g_view;
	viewMTX.unlock();
	return val;
}
#endif

#if LOCAL_FAULT || VIEW_CHANGES
// Server parameters for tracking failed replicas
std::mutex stopMTX[SEND_THREAD_CNT];
vector<vector<uint64_t>> stop_nodes; // List of nodes that have stopped.

// Client parameters for tracking failed replicas.
std::mutex clistopMTX;
vector<uint64_t> stop_replicas; // For client we assume only one O/P thread.
#endif

#if LOCAL_FAULT
uint64_t num_nodes_to_fail = NODE_FAIL_CNT;
#endif

//Statistics global print variables.
double idle_worker_times[THREAD_CNT] = {0};

// Statistics to print output_thread_idle_times.
double output_thd_idle_time[SEND_THREAD_CNT] = {0};
double input_thd_idle_time[REM_THREAD_CNT] = {0};

// Maps for client response couting
SpinLockMap<string, uint64_t> client_responses_count;

// Payload for messages.
#if PAYLOAD_ENABLE
#if PAYLOAD == M100
uint64_t payload_size = 12800;
#elif PAYLOAD == M200
uint64_t payload_size = 25600;
#elif PAYLOAD == M400
uint64_t payload_size = 51200;
#endif
#endif

#if EXT_DB == SQL || EXT_DB == SQL_PERSISTENT
DataBase *db = new SQLite();
#elif EXT_DB == MEMORY
DataBase *db = new InMemoryDB();
#endif

#if SGX

nng_socket enclave_seq_number_sock, enclave_prepare_sock, enclave_commit_sock, enclave_exec_sock;
nng_socket connect_to_enclave(const char *url)
{
	nng_socket sock;
	int rv = -1;

	if ((rv = nng_pair0_open(&sock)))
	{
		cout << nng_strerror(rv);
		fflush(stdout);
	}
	if ((rv = nng_dial(sock, url, NULL, 0)))
	{
		cout << nng_strerror(rv);
		fflush(stdout);
	}
	nng_setopt_ms(sock, NNG_OPT_RECVTIMEO, 100);
	return sock;
}

/**
 * This function is used to verify the sequence number and signature of the TEE
 * verification.
 * @return TEE_VERIFY when the TEE has VERIFIED the message
 * @return TEE_SIGN_INVALID when the TEE has returned FALSE
 */
int tee_verify_counter_and_sign(uint64_t counter, string tee_signature)
{
	return 0;
}
struct enclave_message *send_message_to_enclave(int type)
{
	// Can update any field, and add hash for signing by the TEE; if required for a different consensus protocol.
	// printf("IOThread Generating Sequence called by: %d", g_node_id);

	struct enclave_message *to_send;
	size_t sz;
	to_send = (struct enclave_message *)malloc(sizeof(struct enclave_message));
	to_send->type = type; //asking a counter with signature
	struct enclave_message *seq_buf;
	nng_socket *enclave_socket;

	switch (type)
	{

	case TEE_SEQUENCE_COUNTER:
		enclave_socket = &enclave_seq_number_sock;
		break;
	case TEE_PREPARE_COUNTER:
		enclave_socket = &enclave_prepare_sock;
		break;
	case TEE_COMMIT_COUNTER:
		enclave_socket = &enclave_commit_sock;
		break;
	case TEE_EXECUTE_COUNTER:
		enclave_socket = &enclave_exec_sock;
		break;
	case TEE_TERMINATE:
		enclave_socket = &enclave_seq_number_sock;
		break;
	}
	nng_send(*enclave_socket, to_send, sizeof(struct enclave_message), 0);
	while (nng_recv(*enclave_socket, &seq_buf, &sz, NNG_FLAG_ALLOC) < 0)
	{
		continue;
	}
	if (seq_buf->type != (uint64_t)type)
	{

		printf("seq_buf->type: %ld,  type: %d  socket: %ld \n", seq_buf->type, type, (uint64_t)enclave_socket);
		// fflush(stdout);
		assert(seq_buf->type == (uint64_t)type);
	}
	// printf("Recieved value of sequence: %d \n", val);
	return seq_buf;
	// next_set = val; //ID for Batch
}

#endif
