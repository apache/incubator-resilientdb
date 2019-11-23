#ifndef _STATS_H_
#define _STATS_H_
#include "stats_array.h"

class StatValue
{
public:
    StatValue() : value(0) {}
    void operator+=(double value)
    {
        this->value += value;
    }
    void operator=(double value)
    {
        this->value = value;
    }
    double get() { return value; }

private:
    double value;
};

class Stats_thd
{
public:
    void init(uint64_t thd_id);
    void free();
    void combine(Stats_thd *stats);
    void print(FILE *outf, bool prog);
    void print_client(FILE *outf, bool prog);
    void clear();

    char _pad2[CL_SIZE];

    uint64_t *part_cnt;
    uint64_t *part_acc;

    double total_runtime;

    uint64_t parts_touched;

    // Execution
    uint64_t txn_cnt;
    uint64_t remote_txn_cnt;
    uint64_t local_txn_cnt;
    uint64_t local_txn_start_cnt;
    uint64_t total_txn_commit_cnt;
    uint64_t local_txn_commit_cnt;
    uint64_t remote_txn_commit_cnt;
    uint64_t total_txn_abort_cnt;
    uint64_t unique_txn_abort_cnt;
    uint64_t local_txn_abort_cnt;
    uint64_t remote_txn_abort_cnt;
    double txn_run_time;
    uint64_t multi_part_txn_cnt;
    double multi_part_txn_run_time;
    uint64_t single_part_txn_cnt;
    double single_part_txn_run_time;
    uint64_t txn_write_cnt;
    uint64_t record_write_cnt;

    // Transaction stats
    double txn_total_process_time;
    double txn_process_time;
    double txn_total_local_wait_time;
    double txn_local_wait_time;
    double txn_total_remote_wait_time;
    double txn_remote_wait_time;
    //double txn_total_twopc_time;
    //double txn_twopc_time;

    // Client
    uint64_t txn_sent_cnt;
    double cl_send_intv;

    // Breakdown
    double ts_alloc_time;
    double abort_time;
    double txn_manager_time;
    double txn_index_time;
    double txn_validate_time;
    double txn_cleanup_time;

    // Work queue
    double work_queue_wait_time;
    uint64_t work_queue_cnt;
    uint64_t work_queue_enq_cnt;
    double work_queue_mtx_wait_time;
    uint64_t work_queue_new_cnt;
    double work_queue_new_wait_time;
    uint64_t work_queue_old_cnt;
    double work_queue_old_wait_time;
    double work_queue_enqueue_time;
    double work_queue_dequeue_time;
    uint64_t work_queue_conflict_cnt;

    // Worker thread
    double worker_idle_time;
    double worker_activate_txn_time;
    double worker_deactivate_txn_time;
    double worker_release_msg_time;
    double worker_process_time;
    uint64_t worker_process_cnt;
    uint64_t *worker_process_cnt_by_type;
    double *worker_process_time_by_type;

    // IO
    double msg_queue_delay_time;
    uint64_t msg_queue_cnt;
    uint64_t msg_queue_enq_cnt;
    double msg_send_time;
    double msg_recv_time;
    double msg_recv_idle_time;
    uint64_t msg_batch_cnt;
    uint64_t msg_batch_size_msgs;
    uint64_t msg_batch_size_bytes;
    uint64_t msg_batch_size_bytes_to_server;
    uint64_t msg_batch_size_bytes_to_client;
    uint64_t msg_send_cnt;
    uint64_t msg_recv_cnt;
    double msg_unpack_time;
    double mbuf_send_intv_time;
    double msg_copy_output_time;

    uint64_t bytes_received;
    uint64_t bytes_sent;
    uint64_t client_batch_msg_size;
    uint64_t batch_req_msg_size;
#if CONSENSUS == PBFT
    uint64_t commit_msg_size;
    uint64_t prepare_msg_size;
#endif
    uint64_t checkpoint_msg_size;
    uint64_t client_response_msg_size;
#if GBFT
    uint64_t ccm_msg_size;
#endif
#if CONSENSUS == HOTSTUFF
    uint64_t vote_msg_size;
#endif
#if STEWARD
    uint64_t acc_cert_msg_size;
#endif

#if TIME_PROF_ENABLE
    double *io_thread_idle_time;
#endif
    // Concurrency control, general
    //uint64_t cc_conflict_cnt;
    //uint64_t txn_wait_cnt;
    //uint64_t txn_conflict_cnt;

    // Transaction Table
    uint64_t txn_table_new_cnt;
    uint64_t txn_table_get_cnt;
    uint64_t txn_table_release_cnt;
    uint64_t txn_table_cflt_cnt;
    uint64_t txn_table_cflt_size;
    double txn_table_get_time;
    double txn_table_release_time;
    double txn_table_min_ts_time;

    // Latency
    StatsArr client_client_latency;
    //StatsArr first_start_commit_latency;
    //StatsArr last_start_commit_latency;
    //StatsArr start_abort_commit_latency;

    // stats accumulated
    double lat_work_queue_time;
    double lat_msg_queue_time;
    double lat_cc_block_time;
    double lat_cc_time;
    double lat_process_time;
    double lat_abort_time;
    double lat_network_time;
    double lat_other_time;

    // stats from committed local transactions from the first starttime of the transaction
    double lat_l_loc_work_queue_time;
    double lat_l_loc_msg_queue_time;
    double lat_l_loc_cc_block_time;
    double lat_l_loc_cc_time;
    double lat_l_loc_process_time;
    double lat_l_loc_abort_time;

    // stats from committed local transactions only from the most recent start time
    double lat_s_loc_work_queue_time;
    double lat_s_loc_msg_queue_time;
    double lat_s_loc_cc_block_time;
    double lat_s_loc_cc_time;
    double lat_s_loc_process_time;

    // stats from message-managed latency
    double lat_short_work_queue_time;
    double lat_short_msg_queue_time;
    double lat_short_cc_block_time;
    double lat_short_cc_time;
    double lat_short_process_time;
    double lat_short_network_time;
    double lat_short_batch_time;

    // stats from committed non-local transactions
    double lat_l_rem_work_queue_time;
    double lat_l_rem_msg_queue_time;
    double lat_l_rem_cc_block_time;
    double lat_l_rem_cc_time;
    double lat_l_rem_process_time;

    // stats from aborted non-local transactions
    double lat_s_rem_work_queue_time;
    double lat_s_rem_msg_queue_time;
    double lat_s_rem_cc_block_time;
    double lat_s_rem_cc_time;
    double lat_s_rem_process_time;

    //double * mtx;

    char _pad[CL_SIZE];

    double time_pre_prepare;
    double time_prepare;
    double time_commit;
    double time_execute;
    double tput_msg;
    double msg_cl_in;
    double msg_node_in;
    double msg_cl_out;
    double msg_node_out;
    void set_message_size(uint64_t rtype, uint64_t size);
};

class Stats
{
public:
    // PER THREAD statistics
    Stats_thd **_stats;
    Stats_thd *totals;

    void init(uint64_t thread_cnt);
    void free();
    void clear(uint64_t tid);
    //void add_lat(uint64_t thd_id, uint64_t latency);
    void commit(uint64_t thd_id);
    void abort(uint64_t thd_id);
    void print_client(bool prog);
    void print(bool prog);
    void print_cnts(FILE *outf);
    void print_lat_distr();
    void print_lat_distr(uint64_t min, uint64_t max);
    void print_abort_distr();
    uint64_t get_txn_cnts();
    void util_init();
    void print_util();
    int parseLine(char *line);
    void mem_util(FILE *outf);
    void cpu_util(FILE *outf);
    void print_prof(FILE *outf);

    clock_t lastCPU, lastSysCPU, lastUserCPU;

private:
    uint64_t thd_cnt;
};

#endif
