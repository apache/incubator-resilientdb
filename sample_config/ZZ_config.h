#ifndef _CONFIG_H_
#define _CONFIG_H_
// Specify the number of servers or replicas
#define NODE_CNT 4
// #define THREAD_CNT 5 // This Should be the sum of following thread count + protocol specifig threads
#define WORKER_THREAD_CNT 1
// #define BATCH_THREAD_CNT 2
#define CHECKPOINT_THREAD_CNT 1
#define EXECUTE_THREAD_CNT 1
// IO THREADS
#define REM_THREAD_CNT 2
#define REM_THREAD_CNT_FOR_CLIENTS 1
#define SEND_THREAD_CNT 1
#define CORE_CNT 8
#define PART_CNT 1
// Specify the number of clients.
#define CLIENT_NODE_CNT 1
#define CLIENT_THREAD_CNT 1
#define CLIENT_REM_THREAD_CNT 1 
#define CLIENT_SEND_THREAD_CNT 1
#define CLIENT_RUNTIME false

#define MESSAGE_PER_BUFFER 24
// Hardware Asisted BFT
#define SGX        false
#define A2M        (SGX && false)
#define MIN_PBFT   (SGX && false)
#define MIN_ZYZ    (SGX && false)
#define CONTRAST   (A2M && false)
#define FLEXI_PBFT (SGX && false)
#define FLEXI_ZYZ  (SGX && false)
#define ZYZ        true

#define MIN_PBFT_ALL_TO_ALL false

#define PERSISTENT_COUNTER            SGX &&  false
#define PERS_COUNTER_RESPONSE_TIME    1 * 1000000

#define THREAD_CNT ( (A2M || MIN_PBFT || MIN_ZYZ) && !CONTRAST ? 4 : 5)
#define BATCH_THREAD_CNT ( (A2M || MIN_PBFT || MIN_ZYZ) && !CONTRAST ? 1 : 2)

// GeoBFT Setting 
#define GBFT false
#define GBFT_CLUSTER_SIZE 4
#define GBFT_CCM_THREAD_CNT 1

#define LOAD_PER_SERVER 1
#define REPLICA_CNT 0
#define REPL_TYPE AP
#define VIRTUAL_PART_CNT PART_CNT
#define PAGE_SIZE 4096
#define CL_SIZE 64
#define CPU_FREQ 2.2
#define HW_MIGRATE false
#define WARMUP 0
#define WORKLOAD YCSB
#define PRT_LAT_DISTR false
#define STATS_ENABLE true
#define STATS_DETAILED true
#define STAT_BAND_WIDTH_ENABLE false
#define TIME_ENABLE true
#define TIME_PROF_ENABLE false
#define FIN_BY_TIME true
// Number of transactions each client should send without waiting.
#define MAX_TXN_IN_FLIGHT 7000
#define SERVER_GENERATE_QUERIES false
#define MEM_ALLIGN 8
#define THREAD_ALLOC false
#define THREAD_ARENA_SIZE (1UL << 22)
#define MEM_PAD true
#define PART_ALLOC false
#define MEM_SIZE (1UL << 30)
#define NO_FREE false
#define TPORT_TYPE TCP
#define TPORT_PORT 10000
#define TPORT_WINDOW 20000
#define SET_AFFINITY false
#define MAX_TPORT_NAME 128
#define MSG_SIZE 128
#define HEADER_SIZE sizeof(uint32_t) * 2
#define MSG_TIMEOUT 5 * BILLION // in ns
#define NETWORK_TEST false
#define NETWORK_DELAY_TEST false
#define NETWORK_DELAY 0UL
#define MAX_QUEUE_LEN NODE_CNT * 2
#define MSG_SIZE_MAX 1048576
#define MSG_TIME_LIMIT 0
#define KEY_ORDER false
#define ENABLE_LATCH false
#define CENTRAL_INDEX false
#define CENTRAL_MANAGER false
#define INDEX_STRUCT IDX_HASH
#define BTREE_ORDER 16
#define TS_TWR false
#define TS_BATCH_ALLOC false
#define TS_BATCH_NUM 1
#define HIS_RECYCLE_LEN 10
#define MAX_PRE_REQ MAX_TXN_IN_FLIGHT *NODE_CNT
#define MAX_READ_REQ MAX_TXN_IN_FLIGHT *NODE_CNT
#define MIN_TS_INTVL 10 * 1000000UL
#define MAX_WRITE_SET 10
#define PER_ROW_VALID false
#define TXN_QUEUE_SIZE_LIMIT THREAD_CNT
#define SEQ_THREAD_CNT 4
#define MAX_ROW_PER_TXN 64
#define QUERY_INTVL 1UL
#define MAX_TXN_PER_PART 4000
#define FIRST_PART_LOCAL true
#define MAX_TUPLE_SIZE 1024
#define GEN_BY_MPR false
#define DATA_PERC 100
#define ACCESS_PERC 0.03
#define INIT_PARALLELISM 8
#define SYNTH_TABLE_SIZE 524288

#define ZIPF_THETA 0.5
#define WRITE_PERC 0.9
#define TXN_WRITE_PERC 0.5
#define TUP_WRITE_PERC 0.5
#define PART_PER_TXN PART_CNT
#define PERC_MULTI_PART MPR
#define REQ_PER_QUERY 1
#define FIELD_PER_TUPLE 10
#define STRICT_PPT 1
#define MPR 1.0
#define MPIR 0.01
#define DEBUG_DISTR false
#define DEBUG_ALLOC false
#define DEBUG_RACE false
#define DEBUG_LATENCY false
#define DEBUG_QUECC false
#define DEBUG_WLOAD false
#define DBTYPE REPLICATED
#define IDX_HASH 1
#define IDX_BTREE 2
#define YCSB 1
#define TEST 4
#define ZIPF 1
#define AP 2
#define TCP 1
#define IPC 2
#define BILLION 1000000000UL
#define MILLION 1000000UL
#define STAT_ARR_SIZE 1024
#define PROG_TIMER 5 * BILLION
#define SEQ_BATCH_TIMER 5 * 1 * MILLION
#define SEED 0
#define SHMEM_ENV false
#define ENVIRONMENT_EC2 false
#define PARTITIONED 0
#define REPLICATED 1
// To select the amount of time to warmup and run.
#define DONE_TIMER 60 * BILLION
#define WARMUP_TIMER  15 * BILLION
// Select the consensus algorithm to run.
#define CONSENSUS PBFT
#define PBFT 2
#define ZYZZYVA 3


// Enable or Disable pipeline at primary replica.
#define ENABLE_PIPELINE true
// Size of each batch.
#define BATCH_SIZE 100
#define BATCH_ENABLE BSET
#define BSET 1
#define BUNSET 0
// Number of transactions to wait for period checkpointing.
#define TXN_PER_CHKPT 6 * BATCH_SIZE
#define SIGN_THREADS false
#define CLIENT_BATCH true
#define CLIENT_RESPONSE_BATCH true
// To Enable or disable the blockchain implementation.
#define ENABLE_CHAIN false
// To fail non-primary replicas.
#define LOCAL_FAULT false
#define NODE_FAIL_CNT 1
// To allow view changes.
#define VIEW_CHANGES false
// The amount of timeout value.
#define EXE_TIMEOUT 10000000000
#define CEXE_TIMEOUT 5 * BILLION
// To turn the timer on.
#define TIMER_ON false
//Global variables to choose the encryptation algorithm
#define USE_CRYPTO true
#define CRYPTO_METHOD_RSA false     //Options RSA,
#define CRYPTO_METHOD_ED25519 true  // Option ED25519
#define CRYPTO_METHOD_CMAC_AES true // CMAC<AES>
// Test cases to check basic functioning.
// Status: Implementation only for PBFT.
#define TESTING_ON false
#define TEST_CASE ONLY_PRIMARY_BATCH_EXECUTE
#define ONLY_PRIMARY_NO_EXECUTE 1
#define ONLY_PRIMARY_EXECUTE 2
#define ONLY_PRIMARY_BATCH_EXECUTE 3
// Message Payload.
// We allow creation of two different message payloads,
// to see affects on latency and throughput.
// These payloads are added to each message.
#define PAYLOAD_ENABLE false
#define PAYLOAD M100
#define M100 1 // 100KB.
#define M200 2 // 200KB.
#define M400 3 // 400KB.

// To allow testing in-memory database or SQLite.
// Further, using SQLite a user can also choose to persist the data.
#define EXT_DB MEMORY
#define MEMORY 1
#define SQL 2
#define SQL_PERSISTENT 3

// To allow testing of a Banking Smart Contracts.
#define BANKING_SMART_CONTRACT false

#endif

