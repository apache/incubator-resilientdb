#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <typeinfo>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "./helper.h"
#include "pthread.h"
#include "../config.h"
#include "stats.h"
#include "pool.h"
#include "txn_table.h"
#include "sim_manager.h"
#include <mutex>

#include <unordered_map>
#include "xed25519.h"
#include "sha.h"
#include "database.h"
#include "hash_map.h"
#include "hash_set.h"
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>

using namespace std;

class mem_alloc;
class Stats;
class SimManager;
class Query_queue;
class Transport;
class Remote_query;
class TxnManPool;
class TxnPool;
class TxnTablePool;
class QryPool;
class TxnTable;
class QWorkQueue;
class MessageQueue;
class Client_query_queue;
class Client_txn;
class GeoBFTCommitCertificateMessage;
class PrepCertificateMessage;
class ClientResponseMessage;
class ClientQueryBatch;

typedef uint32_t UInt32;
typedef int32_t SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;

typedef uint64_t ts_t; // time stamp type

/******************************************/
// Global Data Structure
/******************************************/
extern mem_alloc mem_allocator;
extern Stats stats;
extern SimManager *simulation;
extern Query_queue query_queue;
extern Client_query_queue client_query_queue;
extern Transport tport_man;
extern TxnManPool txn_man_pool;
extern TxnPool txn_pool;
extern TxnTablePool txn_table_pool;
extern QryPool qry_pool;
extern TxnTable txn_table;
extern QWorkQueue work_queue;
extern MessageQueue msg_queue;
extern Client_txn client_man;

extern bool volatile warmup_done;
extern bool volatile enable_thread_mem_pool;
extern pthread_barrier_t warmup_bar;

/******************************************/
// Client Global Params
/******************************************/
extern UInt32 g_client_thread_cnt;
extern UInt32 g_client_rem_thread_cnt;
extern UInt32 g_client_send_thread_cnt;
extern UInt32 g_client_node_cnt;
extern UInt32 g_servers_per_client;
extern UInt32 g_clients_per_server;
extern UInt32 g_server_start_node;
extern uint64_t last_valid_txn;
uint64_t get_last_valid_txn();
void set_last_valid_txn(uint64_t txn_id);

/******************************************/
// Global Parameter
/******************************************/
extern volatile UInt64 g_row_id;
extern bool g_part_alloc;
extern bool g_mem_pad;
extern bool g_prt_lat_distr;
extern UInt32 g_node_id;
extern UInt32 g_node_cnt;
extern UInt32 g_part_cnt;
extern UInt32 g_virtual_part_cnt;
extern UInt32 g_core_cnt;
extern UInt32 g_total_node_cnt;
extern UInt32 g_total_thread_cnt;
extern UInt32 g_total_client_thread_cnt;
extern UInt32 g_this_thread_cnt;
extern UInt32 g_this_rem_thread_cnt;
extern UInt32 g_this_send_thread_cnt;
extern UInt32 g_this_total_thread_cnt;
extern UInt32 g_thread_cnt;
extern UInt32 g_worker_thread_cnt;
extern UInt32 g_batching_thread_cnt;
extern UInt32 g_checkpointing_thread_cnt;
extern UInt32 g_execution_thread_cnt;

extern UInt32 g_sign_thd;
extern UInt32 g_send_thread_cnt;
extern UInt32 g_rem_thread_cnt;

extern bool g_key_order;
extern bool g_ts_batch_alloc;
extern UInt32 g_ts_batch_num;
extern int32_t g_inflight_max;
extern uint64_t g_msg_size;

extern UInt32 g_max_txn_per_part;
extern int32_t g_load_per_server;

extern bool g_hw_migrate;
extern UInt32 g_network_delay;
extern UInt64 g_done_timer;
extern UInt64 g_batch_time_limit;
extern UInt64 g_seq_batch_time_limit;
extern UInt64 g_prog_timer;
extern UInt64 g_warmup_timer;
extern UInt64 g_msg_time_limit;

// YCSB
extern ts_t g_query_intvl;
extern UInt32 g_part_per_txn;
extern double g_perc_multi_part;
extern double g_txn_read_perc;
extern double g_txn_write_perc;
extern double g_tup_read_perc;
extern double g_tup_write_perc;
extern double g_zipf_theta;
extern double g_data_perc;
extern double g_access_perc;
extern UInt64 g_synth_table_size;
extern UInt32 g_req_per_query;
extern bool g_strict_ppt;
extern UInt32 g_field_per_tuple;
extern UInt32 g_init_parallelism;
extern double g_mpr;
extern double g_mpitem;

extern DataBase *db;

// Replication
extern UInt32 g_repl_type;
extern UInt32 g_repl_cnt;

enum RC
{
    RCOK = 0,
    Commit,
    FINISH,
    NONE
};

// Identifiers for Different Message types.
enum RemReqType
{
    INIT_DONE = 0,
    KEYEX,
    READY,
    CL_QRY, // Client Query
    RTXN,
    RTXN_CONT,
    RINIT,
    RDONE,
    CL_RSP, // Client Response
#if CLIENT_BATCH
    CL_BATCH, // Client Batch
#endif
    NO_MSG,

    EXECUTE_MSG, // Execute Notification
    BATCH_REQ,   // Pre-Prepare

#if VIEW_CHANGES
    VIEW_CHANGE,
    NEW_VIEW,
#endif
#if BANKING_SMART_CONTRACT
    BSC_MSG,
#endif

#if MIN_PBFT_ALL_TO_ALL
    PREP_CERTIFICATE_MSG,
#endif
#if GBFT
    GBFT_COMMIT_CERTIFICATE_MSG,
#endif
    PBFT_PREP_MSG,   // Prepare
    PBFT_COMMIT_MSG, // Commit
    PBFT_CHKPT_MSG   // Checkpoint and Garbage Collection

};

/* Thread */
typedef uint64_t txnid_t;

/* Txn */
typedef uint64_t txn_t;

typedef uint64_t idx_key_t;              // key id for index
typedef uint64_t (*func_ptr)(idx_key_t); // part_id func_ptr(index_key);

/* general concurrency control */
enum access_t
{
    RD,
    WR,
    XP,
    SCAN
};

#define GET_THREAD_ID(id) (id % g_thread_cnt)
#define GET_NODE_ID(id) (id % g_node_cnt)
#define GET_PART_ID(t, n) (n)
#define GET_PART_ID_FROM_IDX(idx) (g_node_id + idx * g_node_cnt)
#define GET_PART_ID_IDX(p) (p / g_node_cnt)
#define ISSERVER (g_node_id < g_node_cnt)
#define ISSERVERN(id) (id < g_node_cnt)
#define ISCLIENT (g_node_id >= g_node_cnt && g_node_id < g_node_cnt + g_client_node_cnt)
#define ISREPLICA (g_node_id >= g_node_cnt + g_client_node_cnt && g_node_id < g_node_cnt + g_client_node_cnt + g_repl_cnt * g_node_cnt)
#define ISREPLICAN(id) (id >= g_node_cnt + g_client_node_cnt && id < g_node_cnt + g_client_node_cnt + g_repl_cnt * g_node_cnt)
#define ISCLIENTN(id) (id >= g_node_cnt && id < g_node_cnt + g_client_node_cnt)

//@Suyash
#if DBTYPE != REPLICATED
#define IS_LOCAL(tid) (tid % g_node_cnt == g_node_id)
#else
#define IS_LOCAL(tid) true
#endif

#define IS_REMOTE(tid) (tid % g_node_cnt != g_node_id)
#define IS_LOCAL_KEY(key) (key % g_node_cnt == g_node_id)

#define MSG(str, args...)                                   \
    {                                                       \
        printf("[%s : %d] " str, __FILE__, __LINE__, args); \
    }

// principal index structure. The workload may decide to use a different
// index structure for specific purposes. (e.g. non-primary key access should use hash)
#if (INDEX_STRUCT == IDX_BTREE)
#define INDEX index_btree
#else // IDX_HASH
#define INDEX IndexHash
#endif

/************************************************/
// constants
/************************************************/
#ifndef UINT64_MAX
#define UINT64_MAX 18446744073709551615UL
#endif // UINT64_MAX

/******** Key storage for signing. ***********/
//ED25519 and RSA
extern string g_priv_key;                             //stores this node's private key
extern string g_public_key;                           //stores this node's public key
extern string g_pub_keys[NODE_CNT + CLIENT_NODE_CNT]; //stores public keys of other nodes
//CMAC
extern string cmacPrivateKeys[NODE_CNT + CLIENT_NODE_CNT];
extern string cmacOthersKeys[NODE_CNT + CLIENT_NODE_CNT];
// ED25519
extern CryptoPP::ed25519::Verifier verifier[NODE_CNT + CLIENT_NODE_CNT];
extern CryptoPP::ed25519::Signer signer;

// Receiving keys
extern uint64_t receivedKeys[NODE_CNT + CLIENT_NODE_CNT];

//Types for Keypair
typedef unsigned char byte;
#define copyStringToByte(byte, str)           \
    for (uint64_t i = 0; i < str.size(); i++) \
        byte[i] = str[i];
struct KeyPairHex
{
    std::string publicKey;
    std::string privateKey;
};

/*********************************************/

extern std::mutex keyMTX;
extern bool keyAvail;
extern uint64_t totKey;

extern uint64_t indexSize;
extern uint64_t g_min_invalid_nodes;

// Funtion to calculate hash of a string.
string calculateHash(string str);

// Entities for maintaining g_next_index.
extern uint64_t g_next_index; //index of the next txn to be executed
extern std::mutex gnextMTX;
void inc_next_index();
uint64_t curr_next_index();

// Entities for handling checkpoints.
extern uint32_t g_last_stable_chkpt; //index of the last stable checkpoint
void set_curr_chkpt(uint64_t txn_id);
uint64_t get_curr_chkpt();

extern uint32_t g_txn_per_chkpt; // fixed interval of collecting checkpoints.
uint64_t txn_per_chkpt();

extern uint64_t lastDeletedTxnMan; // index of last deleted txn manager.
void inc_last_deleted_txn();
uint64_t get_last_deleted_txn();

extern uint64_t expectedExecuteCount;
extern uint64_t expectedCheckpoint;
uint64_t get_expectedExecuteCount();
void set_expectedExecuteCount(uint64_t val);

// Variable used by all threads during setup, to mark they are ready
extern std::mutex batchMTX;
extern uint commonVar;

// Variable used by Input thread at the primary to linearize batches.
extern uint64_t next_idx;
#if PERSISTENT_COUNTER
extern uint64_t per_counter_last_response_time;
#endif

#if PERSISTENT_COUNTER
bool get_and_inc_next_idx(ClientQueryBatch *clbatch, uint64_t &id);
#elif SGX
uint64_t get_and_inc_next_idx(ClientQueryBatch *clbatch);
#else
uint64_t get_and_inc_next_idx();
#endif
void set_next_idx(uint64_t val);

// Counters for input threads to access next socket (only used by replicas).
extern uint64_t sock_ctr[REM_THREAD_CNT];
uint64_t get_next_socket(uint64_t tid, uint64_t size);

// Global Utility functions:
vector<uint64_t> nodes_to_send(uint64_t beg, uint64_t end); // Destination for msgs.

// STORAGE OF CLIENT DATA
extern uint64_t ClientDataStore[SYNTH_TABLE_SIZE];

// Entities related to RBFT protocol.

// Entities pertaining to the current view.
uint64_t get_current_view(uint64_t thd_id);
uint64_t get_view(uint64_t thd_id);

#if VIEW_CHANGES
// For updating view for input threads, batching threads, execute thread
// and checkpointing thread.
extern std::mutex newViewMTX[THREAD_CNT + REM_THREAD_CNT + SEND_THREAD_CNT];
extern uint64_t view[THREAD_CNT + REM_THREAD_CNT + SEND_THREAD_CNT];
void set_view(uint64_t thd_id, uint64_t val);
#endif

// Size of the batch.
extern uint64_t g_batch_size;
uint64_t get_batch_size();
extern uint64_t batchSet[2 * CLIENT_NODE_CNT * MAX_TXN_IN_FLIGHT];
uint64_t view_to_primary(uint64_t view, uint64_t node = g_node_id);

#if GBFT
extern uint32_t g_view[];
extern std::mutex viewMTX[];
uint64_t get_cluster_number(uint64_t i = g_node_id);
void set_client_view(uint64_t nview, int shard = 0);
uint64_t get_client_view(int cluster = 0);
int is_in_same_cluster(uint64_t first_id, uint64_t second_id);
bool is_local_request(TxnManager *tman);
bool is_primary_node(uint64_t thd_id, uint64_t node = g_node_id);

// void set_view(uint64_t nview, int cluster = 0);
// uint64_t get_view(int cluster = 0);

extern uint32_t gbft_last_commited_txn;
extern UInt32 gbft_cluster_size;
extern UInt32 gbft_cluster_cnt;
extern UInt32 gbft_commit_certificate_thread_cnt;
extern SpinLockSet<string> gbft_ccm_checklist;

#else
// This variable is mainly used by the client to know its current primary.
extern uint32_t g_view;
extern std::mutex viewMTX;
void set_client_view(uint64_t nview);
uint64_t get_client_view();
#endif

#if LOCAL_FAULT || VIEW_CHANGES
// Server parameters for tracking failed replicas
extern std::mutex stopMTX[SEND_THREAD_CNT];
extern vector<vector<uint64_t>> stop_nodes; // List of nodes that have stopped.

// Client parameters for tracking failed replicas.
extern std::mutex clistopMTX;
extern vector<uint64_t> stop_replicas; // For client we assume only one O/P thread.
#endif

#if LOCAL_FAULT
extern uint64_t num_nodes_to_fail;
#endif

//Statistics global print variables -- only used in stats.cpp.
extern double idle_worker_times[THREAD_CNT];

// Statistics to print output_thread_idle_times.
extern double output_thd_idle_time[SEND_THREAD_CNT];
extern double input_thd_idle_time[REM_THREAD_CNT];

// Maps for client response couting
extern SpinLockMap<string, uint64_t> client_responses_count;

// Payload for messages.
#if PAYLOAD_ENABLE
extern uint64_t payload_size;
#endif

#if BANKING_SMART_CONTRACT
enum BSCType
{
    BSC_TRANSFER = 0,
    BSC_DEPOSIT = 1,
    BSC_WITHDRAW = 2,
};
#endif

#if SGX

#define TEE_SEQUENCE_COUNTER 0
#define TEE_VERIFY 1
#define TEE_PREPARE_COUNTER 2
#define TEE_COMMIT_COUNTER 3
#define TEE_EXECUTE_COUNTER 4
#define TEE_TERMINATE 10
struct enclave_message
{
    uint64_t type;
    uint64_t counter;
    uint32_t signature[16]; //Signature size: check
};
extern nng_socket enclave_seq_number_sock, enclave_prepare_sock, enclave_commit_sock, enclave_exec_sock;
nng_socket connect_to_enclave(const char *url);
int tee_verify_counter_and_sign(uint64_t counter, string tee_signature);
struct enclave_message *send_message_to_enclave(int type);

#endif // SGX

#endif
