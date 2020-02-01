#!/bin/bash

nodes=$1
cnodes=$2
[ ! -z "$3" ] && max_inf=$3 || max_inf="20000"

consensus="PBFT"

echo -e "#ifndef _CONFIG_H_ " >config.h
echo -e "#define _CONFIG_H_ " >>config.h
echo -e "// Specify the number of servers or replicas " >>config.h
echo -e "#define NODE_CNT ${nodes} " >>config.h
echo -e "// Number of worker threads at primary. For RBFT (6) and other algorithms (5). " >>config.h
echo -e "#define THREAD_CNT 5 " >>config.h
echo -e "#define REM_THREAD_CNT 3" >>config.h
echo -e "#define SEND_THREAD_CNT 1 " >>config.h
echo -e "#define CORE_CNT 8 " >>config.h
echo -e "#define PART_CNT 1 " >>config.h
echo -e "// Specify the number of clients. " >>config.h
echo -e "#define CLIENT_NODE_CNT ${cnodes}" >>config.h
echo -e "#define CLIENT_THREAD_CNT 2 " >>config.h
echo -e "#define CLIENT_REM_THREAD_CNT 1 " >>config.h
echo -e "#define CLIENT_SEND_THREAD_CNT 1 " >>config.h
echo -e "#define CLIENT_RUNTIME false  " >>config.h
echo -e "#define LOAD_PER_SERVER 1 " >>config.h
echo -e "#define REPLICA_CNT 0 " >>config.h
echo -e "#define REPL_TYPE AP " >>config.h
echo -e "#define VIRTUAL_PART_CNT PART_CNT " >>config.h
echo -e "#define PAGE_SIZE 4096 " >>config.h
echo -e "#define CL_SIZE 64 " >>config.h
echo -e "#define CPU_FREQ 2.6 " >>config.h
echo -e "#define HW_MIGRATE false " >>config.h
echo -e "#define WARMUP 0 " >>config.h
echo -e "#define WORKLOAD YCSB " >>config.h
echo -e "#define PRT_LAT_DISTR false " >>config.h
echo -e "#define STATS_ENABLE true " >>config.h
echo -e "#define TIME_ENABLE true " >>config.h
echo -e "#define TIME_PROF_ENABLE false " >>config.h
echo -e "#define FIN_BY_TIME true " >>config.h
echo -e "// Number of transactions each client should send without waiting. " >>config.h
echo -e "#define MAX_TXN_IN_FLIGHT $max_inf " >>config.h
echo -e "#define SERVER_GENERATE_QUERIES false  " >>config.h
echo -e "#define MEM_ALLIGN 8 " >>config.h
echo -e "#define THREAD_ALLOC false " >>config.h
echo -e "#define THREAD_ARENA_SIZE (1UL << 22) " >>config.h
echo -e "#define MEM_PAD true " >>config.h
echo -e "#define PART_ALLOC false " >>config.h
echo -e "#define MEM_SIZE (1UL << 30) " >>config.h
echo -e "#define NO_FREE false " >>config.h
echo -e "#define TPORT_TYPE TCP " >>config.h
echo -e "#define TPORT_PORT 17000 " >>config.h
echo -e "#define SET_AFFINITY false " >>config.h
echo -e "#define MAX_TPORT_NAME 128 " >>config.h
echo -e "#define MSG_SIZE 128 " >>config.h
echo -e "#define HEADER_SIZE sizeof(uint32_t)*2 " >>config.h
echo -e "#define MSG_TIMEOUT 5000000000UL  // in ns " >>config.h
echo -e "#define NETWORK_TEST false " >>config.h
echo -e "#define NETWORK_DELAY_TEST false " >>config.h
echo -e "#define NETWORK_DELAY 0UL " >>config.h
echo -e "#define MAX_QUEUE_LEN NODE_CNT * 2 " >>config.h
echo -e "#define PRIORITY_WORK_QUEUE false " >>config.h
echo -e "#define PRIORITY PRIORITY_ACTIVE " >>config.h
echo -e "#define MSG_SIZE_MAX 1048576 " >>config.h
echo -e "#define MSG_TIME_LIMIT 0 " >>config.h
echo -e "#define KEY_ORDER false " >>config.h
echo -e "#define ENABLE_LATCH false " >>config.h
echo -e "#define CENTRAL_INDEX false " >>config.h
echo -e "#define CENTRAL_MANAGER false " >>config.h
echo -e "#define INDEX_STRUCT IDX_HASH " >>config.h
echo -e "#define BTREE_ORDER 16 " >>config.h
echo -e "#define TS_TWR false " >>config.h
echo -e "#define TS_ALLOC TS_CLOCK " >>config.h
echo -e "#define TS_BATCH_ALLOC false " >>config.h
echo -e "#define TS_BATCH_NUM 1 " >>config.h
echo -e "#define HIS_RECYCLE_LEN 10 " >>config.h
echo -e "#define MAX_PRE_REQ         MAX_TXN_IN_FLIGHT * NODE_CNT " >>config.h
echo -e "#define MAX_READ_REQ        MAX_TXN_IN_FLIGHT * NODE_CNT " >>config.h
echo -e "#define MIN_TS_INTVL        10 * 1000000UL " >>config.h
echo -e "#define MAX_WRITE_SET       10 " >>config.h
echo -e "#define PER_ROW_VALID       false " >>config.h
echo -e "#define TXN_QUEUE_SIZE_LIMIT    THREAD_CNT " >>config.h
echo -e "#define SEQ_THREAD_CNT 4 " >>config.h
echo -e "#define MAX_ROW_PER_TXN       64 " >>config.h
echo -e "#define QUERY_INTVL         1UL " >>config.h
echo -e "#define MAX_TXN_PER_PART 4000 " >>config.h
echo -e "#define FIRST_PART_LOCAL      true " >>config.h
echo -e "#define MAX_TUPLE_SIZE        1024 " >>config.h
echo -e "#define GEN_BY_MPR false " >>config.h
echo -e "#define SKEW_METHOD ZIPF " >>config.h
echo -e "#define DATA_PERC 100 " >>config.h
echo -e "#define ACCESS_PERC 0.03 " >>config.h
echo -e "#define INIT_PARALLELISM 8 " >>config.h
echo -e "#define SYNTH_TABLE_SIZE 524288" >>config.h
echo -e "" >>config.h
echo -e "#define ZIPF_THETA 0.5 " >>config.h
echo -e "#define WRITE_PERC 0.9 " >>config.h
echo -e "#define TXN_WRITE_PERC 0.5 " >>config.h
echo -e "#define TUP_WRITE_PERC 0.5 " >>config.h
echo -e "#define SCAN_PERC           0 " >>config.h
echo -e "#define SCAN_LEN          20 " >>config.h
echo -e "#define PART_PER_TXN PART_CNT " >>config.h
echo -e "#define PERC_MULTI_PART MPR " >>config.h
echo -e "#define REQ_PER_QUERY 1 " >>config.h
echo -e "#define FIELD_PER_TUPLE       10 " >>config.h
echo -e "#define CREATE_TXN_FILE false " >>config.h
echo -e "#define SINGLE_THREAD_WL_GEN true " >>config.h
echo -e "#define STRICT_PPT 1 " >>config.h
echo -e "#define MPR 1.0 " >>config.h
echo -e "#define MPIR 0.01 " >>config.h
echo -e "#define WL_VERB           true " >>config.h
echo -e "#define IDX_VERB          false " >>config.h
echo -e "#define VERB_ALLOC          true " >>config.h
echo -e "#define DEBUG_LOCK          false " >>config.h
echo -e "#define DEBUG_TIMESTAMP       false " >>config.h
echo -e "#define DEBUG_SYNTH         false " >>config.h
echo -e "#define DEBUG_ASSERT        false " >>config.h
echo -e "#define DEBUG_DISTR false " >>config.h
echo -e "#define DEBUG_ALLOC false " >>config.h
echo -e "#define DEBUG_RACE false " >>config.h
echo -e "#define DEBUG_TIMELINE        false " >>config.h
echo -e "#define DEBUG_BREAKDOWN       false " >>config.h
echo -e "#define DEBUG_LATENCY       false " >>config.h
echo -e "#define DEBUG_QUECC false " >>config.h
echo -e "#define DEBUG_WLOAD false " >>config.h
echo -e "#define MODE NORMAL_MODE " >>config.h
echo -e "#define DBTYPE REPLICATED " >>config.h
echo -e "#define IDX_HASH          1 " >>config.h
echo -e "#define IDX_BTREE         2 " >>config.h
echo -e "#define YCSB            1 " >>config.h
echo -e "#define TEST            4 " >>config.h
echo -e "#define TS_MUTEX          1 " >>config.h
echo -e "#define TS_CAS            2 " >>config.h
echo -e "#define TS_HW           3 " >>config.h
echo -e "#define TS_CLOCK          4 " >>config.h
echo -e "#define ZIPF 1 " >>config.h
echo -e "#define HOT 2 " >>config.h
echo -e "#define PRIORITY_FCFS 1 " >>config.h
echo -e "#define PRIORITY_ACTIVE 2 " >>config.h
echo -e "#define PRIORITY_HOME 3 " >>config.h
echo -e "#define AA1 1 " >>config.h
echo -e "#define AP 2 " >>config.h
echo -e "#define LOAD_MAX 1 " >>config.h
echo -e "#define LOAD_RATE 2 " >>config.h
echo -e "#define TCP 1 " >>config.h
echo -e "#define IPC 2 " >>config.h
echo -e "#define BILLION 1000000000UL " >>config.h
echo -e "#define MILLION 1000000UL " >>config.h
echo -e "#define STAT_ARR_SIZE 1024 " >>config.h
echo -e "#define PROG_TIMER 10 * BILLION " >>config.h
echo -e "#define SEQ_BATCH_TIMER 5 * 1 * MILLION " >>config.h
echo -e "#define SEED 0 " >>config.h
echo -e "#define SHMEM_ENV false " >>config.h
echo -e "#define ENVIRONMENT_EC2 false  " >>config.h
echo -e "#define PARTITIONED 0 " >>config.h
echo -e "#define REPLICATED 1 " >>config.h
echo -e "// To select the amount of time to warmup and run. " >>config.h
echo -e "#define DONE_TIMER 2 * 60 * BILLION " >>config.h
echo -e "#define WARMUP_TIMER 1 * 60 * BILLION " >>config.h
echo -e "// Select the consensus algorithm to run.  " >>config.h
echo -e "#define CONSENSUS ${consensus}" >>config.h
echo -e "#define DBFT 1 " >>config.h
echo -e "#define PBFT 2 " >>config.h
echo -e "#define ZYZZYVA 3 " >>config.h
echo -e "#define HOTSTUFF 4 " >>config.h
echo -e "// Switching on RBFT consensus. " >>config.h
echo -e "// Status: Partial implementation, only for PBFT. " >>config.h
echo -e "#define RBFT_ON false " >>config.h
echo -e "// Select the type of RBFT, (1) RBFT+PBFT, and  (2) RBFT+DBFT " >>config.h
echo -e "#define RBFT_ALG RPBFT " >>config.h
echo -e "#define RPBFT 1 " >>config.h
echo -e "#define RDBFT 2 " >>config.h
echo -e "// Enable or Disable pipeline at primary replica. " >>config.h
echo -e "#define ENABLE_PIPELINE true " >>config.h
echo -e "// Number of threads to create batches at primary replica.  " >>config.h
echo -e "#define BATCH_THREADS 2 " >>config.h
echo -e "// Size of each batch. " >>config.h
echo -e "#define BATCH_SIZE 100 " >>config.h
echo -e "#define BATCH_ENABLE BSET " >>config.h
echo -e "#define BSET 1 " >>config.h
echo -e "#define BUNSET 0 " >>config.h
echo -e "// Number of transactions to wait for period checkpointing. " >>config.h
echo -e "#define TXN_PER_CHKPT 600 " >>config.h
echo -e "#define EXECUTION_THREAD true " >>config.h
echo -e "#define EXECUTE_THD_CNT 1 " >>config.h
echo -e "#define SIGN_THREADS false " >>config.h
echo -e "#define SIGN_THD_CNT 1 " >>config.h
echo -e "#define CLIENT_BATCH true " >>config.h
echo -e "#define CLIENT_RESPONSE_BATCH true " >>config.h
echo -e "// To Enable or disable the blockchain implementation." >>config.h
echo -e "#define ENABLE_CHAIN true" >>config.h
echo -e "// To fail non-primary replicas. " >>config.h
echo -e "#define LOCAL_FAULT false " >>config.h
echo -e "#define NODE_FAIL_CNT 1 " >>config.h
echo -e "// To allow view changes. " >>config.h
echo -e "#define VIEW_CHANGES false " >>config.h
echo -e "// The amount of timeout value. " >>config.h
echo -e "#define EXE_TIMEOUT  10000000000  " >>config.h
echo -e "#define CEXE_TIMEOUT 12000000000  " >>config.h
echo -e "// To turn the timer on. " >>config.h
echo -e "#define TIMER_ON false " >>config.h
echo -e "//Global variables to choose the encryptation algorithm " >>config.h
echo -e "#define USE_CRYPTO true " >>config.h
echo -e "#define CRYPTO_METHOD_RSA false //Options RSA,  " >>config.h
echo -e "#define CRYPTO_METHOD_ED25519 true // Option ED25519 " >>config.h
echo -e "#define CRYPTO_METHOD_CMAC_AES true // CMAC<AES> " >>config.h
echo -e "// Test cases to check basic functioning. " >>config.h
echo -e "// Status: Implementation only for PBFT. " >>config.h
echo -e "#define TESTING_ON false " >>config.h
echo -e "#define TEST_CASE ONLY_PRIMARY_BATCH_EXECUTE " >>config.h
echo -e "#define ONLY_PRIMARY_NO_EXECUTE 1 " >>config.h
echo -e "#define ONLY_PRIMARY_EXECUTE 2 " >>config.h
echo -e "#define ONLY_PRIMARY_BATCH_EXECUTE 3 " >>config.h
echo -e "// Message Payload. " >>config.h
echo -e "// We allow creation of two different message payloads, " >>config.h
echo -e "// to see affects on latency and throughput. " >>config.h
echo -e "// These payloads are added to each message. " >>config.h
echo -e "#define PAYLOAD_ENABLE false " >>config.h
echo -e "#define PAYLOAD M100 " >>config.h
echo -e "#define M100 1	// 100KB. " >>config.h
echo -e "#define M200 2	// 200KB. " >>config.h
echo -e "#define M400 3	// 400KB. " >>config.h
echo -e "" >>config.h
echo -e "#define EXT_DB SQL" >> config.h
echo -e "#define MEMORY 1" >> config.h
echo -e "#define SQL 2" >> config.h
echo -e "#define SQL_PERSISTENT 3" >> config.h
echo -e "" >>config.h
echo -e "#endif" >>config.h
echo -e "" >>config.h
