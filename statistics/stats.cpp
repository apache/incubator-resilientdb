#include "global.h"
#include "helper.h"
#include "stats.h"
#include "mem_alloc.h"
#include "client_txn.h"
#include "work_queue.h"
#include "stats_array.h"
#include <time.h>
#include <sys/times.h>
#include <sys/vtimes.h>

void Stats_thd::init(uint64_t thd_id)
{
    DEBUG_M("Stats_thd::init part_cnt alloc\n");
    part_cnt = (uint64_t *)mem_allocator.align_alloc(sizeof(uint64_t) * g_part_cnt);
    DEBUG_M("Stats_thd::init part_acc alloc\n");
    part_acc = (uint64_t *)mem_allocator.align_alloc(sizeof(uint64_t) * g_part_cnt);
    DEBUG_M("Stats_thd::init worker_process_cnt_by_type alloc\n");
    worker_process_cnt_by_type = (uint64_t *)mem_allocator.align_alloc(sizeof(uint64_t) * NO_MSG);
    DEBUG_M("Stats_thd::init worker_process_time_by_type alloc\n");
    worker_process_time_by_type = (double *)mem_allocator.align_alloc(sizeof(double) * NO_MSG);
#if TIME_PROF_ENABLE
    io_thread_idle_time = (double *)mem_allocator.align_alloc(sizeof(double) * (g_send_thread_cnt + g_rem_thread_cnt));
#endif

    client_client_latency.init(g_max_txn_per_part, ArrIncr);
    clear();
}

void Stats_thd::free()
{
    mem_allocator.free(worker_process_time_by_type, 0);
    mem_allocator.free(worker_process_cnt_by_type, 0);
    mem_allocator.free(part_acc, 0);
    mem_allocator.free(part_cnt, 0);

#if TIME_PROF_ENABLE
    mem_allocator.free(io_thread_idle_time, 0);
#endif

    client_client_latency.free();
    //last_start_commit_latency.free();
    //first_start_commit_latency.free();
    //start_abort_commit_latency.free();
}

void Stats_thd::clear()
{

    total_runtime = 0;

    // Execution
    txn_cnt = 0;
    remote_txn_cnt = 0;
    local_txn_cnt = 0;
    local_txn_start_cnt = 0;
    total_txn_commit_cnt = 0;
    local_txn_commit_cnt = 0;
    remote_txn_commit_cnt = 0;
    total_txn_abort_cnt = 0;
    unique_txn_abort_cnt = 0;
    local_txn_abort_cnt = 0;
    remote_txn_abort_cnt = 0;
    txn_run_time = 0;
    multi_part_txn_cnt = 0;
    multi_part_txn_run_time = 0;
    single_part_txn_cnt = 0;
    single_part_txn_run_time = 0;
    txn_write_cnt = 0;
    record_write_cnt = 0;
    parts_touched = 0;

    // Breakdown
    ts_alloc_time = 0;
    abort_time = 0;
    txn_manager_time = 0;
    txn_index_time = 0;
    txn_validate_time = 0;
    txn_cleanup_time = 0;

    // PBFT
#if CONSENSUS == PBFT || CONSENSUS == DBFT
    time_pre_prepare = 0;
    time_prepare = 0;
    time_commit = 0;
    time_execute = 0;
    tput_msg = 0;
    msg_cl_in = 0;
    msg_node_in = 0;
    msg_cl_out = 0;
    msg_node_out = 0;
#endif

    // Transaction stats
    txn_total_process_time = 0;
    txn_process_time = 0;
    txn_total_local_wait_time = 0;
    txn_local_wait_time = 0;
    txn_total_remote_wait_time = 0;
    txn_remote_wait_time = 0;

    // Client
    txn_sent_cnt = 0;
    cl_send_intv = 0;

    // Work queue
    work_queue_wait_time = 0;
    work_queue_cnt = 0;
    work_queue_enq_cnt = 0;
    work_queue_mtx_wait_time = 0;
    work_queue_new_cnt = 0;
    work_queue_new_wait_time = 0;
    work_queue_old_cnt = 0;
    work_queue_old_wait_time = 0;
    work_queue_enqueue_time = 0;
    work_queue_dequeue_time = 0;
    work_queue_conflict_cnt = 0;

    // Worker thread
    worker_idle_time = 0;
    worker_activate_txn_time = 0;
    worker_deactivate_txn_time = 0;
    worker_release_msg_time = 0;
    worker_process_time = 0;
    worker_process_cnt = 0;
    for (uint64_t i = 0; i < NO_MSG; i++)
    {
        worker_process_cnt_by_type[i] = 0;
        worker_process_time_by_type[i] = 0;
    }

    // IO
    msg_queue_delay_time = 0;
    msg_queue_cnt = 0;
    msg_queue_enq_cnt = 0;
    msg_send_time = 0;
    msg_recv_time = 0;
    msg_recv_idle_time = 0;
    msg_batch_cnt = 0;
    msg_batch_size_msgs = 0;
    msg_batch_size_bytes = 0;
    msg_batch_size_bytes_to_server = 0;
    msg_batch_size_bytes_to_client = 0;
    msg_send_cnt = 0;
    msg_recv_cnt = 0;
    msg_unpack_time = 0;
    mbuf_send_intv_time = 0;
    msg_copy_output_time = 0;
#if TIME_PROF_ENABLE
    for (uint64_t i = 0; i < (g_send_thread_cnt + g_rem_thread_cnt); ++i)
    {
        io_thread_idle_time[i] = 0;
    }
#endif

    // Transaction Table
    txn_table_new_cnt = 0;
    txn_table_get_cnt = 0;
    txn_table_release_cnt = 0;
    txn_table_cflt_cnt = 0;
    txn_table_cflt_size = 0;
    txn_table_get_time = 0;
    txn_table_release_time = 0;
    txn_table_min_ts_time = 0;

    //for(uint64_t i = 0; i < 40; i ++) {
    //  mtx[i]=0;
    //}

    lat_work_queue_time = 0;
    lat_msg_queue_time = 0;
    lat_cc_block_time = 0;
    lat_cc_time = 0;
    lat_process_time = 0;
    lat_abort_time = 0;
    lat_network_time = 0;
    lat_other_time = 0;

    lat_l_loc_work_queue_time = 0;
    lat_l_loc_msg_queue_time = 0;
    lat_l_loc_cc_block_time = 0;
    lat_l_loc_cc_time = 0;
    lat_l_loc_process_time = 0;
    lat_l_loc_abort_time = 0;

    lat_short_work_queue_time = 0;
    lat_short_msg_queue_time = 0;
    lat_short_cc_block_time = 0;
    lat_short_cc_time = 0;
    lat_short_process_time = 0;
    lat_short_network_time = 0;
    lat_short_batch_time = 0;

    lat_s_loc_work_queue_time = 0;
    lat_s_loc_msg_queue_time = 0;
    lat_s_loc_cc_block_time = 0;
    lat_s_loc_cc_time = 0;
    lat_s_loc_process_time = 0;

    lat_l_rem_work_queue_time = 0;
    lat_l_rem_msg_queue_time = 0;
    lat_l_rem_cc_block_time = 0;
    lat_l_rem_cc_time = 0;
    lat_l_rem_process_time = 0;

    lat_s_rem_work_queue_time = 0;
    lat_s_rem_msg_queue_time = 0;
    lat_s_rem_cc_block_time = 0;
    lat_s_rem_cc_time = 0;
    lat_s_rem_process_time = 0;

    client_client_latency.clear();
    //last_start_commit_latency.clear();
    //first_start_commit_latency.clear();
    //start_abort_commit_latency.clear();
}

void Stats_thd::print_client(FILE *outf, bool prog)
{
    string node_id = to_string(g_node_cnt -  g_node_id + 1);
    string filename = "toInflux_C";
    filename.append(node_id);
    filename.append("_.out");
    filename="./monitor/"+filename;
    
    ofstream file;
    file.open(filename.c_str(), ios_base::out | ios_base::in);  // will not create file
    fprintf(outf, "filename: %s", filename.c_str());
    if (!file.is_open())
    {    
        file.clear();
        file.open(filename.c_str(), std::ofstream::app);  // will create if necessary
        //file << ip_add;
        file << "tput\n";
    } 
    else
    {
        file.close();
        file.open (filename.c_str(),std::ofstream::app);
    }

    double txn_run_avg_time = 0;
    double tput = 0;
    if (txn_cnt > 0)
        txn_run_avg_time = txn_run_time / txn_cnt;
    if (total_runtime > 0)
        tput = txn_cnt / (total_runtime / BILLION);

    file << tput;
    file << "\n";
    file.close();

    fprintf(outf,
            "total_runtime=%f"
            ",tput=%f"
            ",txn_cnt=%ld"
            ",txn_sent_cnt=%ld"
            ",txn_run_time=%f"
            ",txn_run_avg_time=%f"
            ",cl_send_intv=%f",
            total_runtime / BILLION, tput, txn_cnt, txn_sent_cnt, txn_run_time / BILLION, txn_run_avg_time / BILLION, cl_send_intv / BILLION);
    // IO
    double mbuf_send_intv_time_avg = 0;
    double msg_unpack_time_avg = 0;
    double msg_send_time_avg = 0;
    double msg_recv_time_avg = 0;
    double msg_batch_size_msgs_avg = 0;
    double msg_batch_size_bytes_avg = 0;
    double msg_queue_delay_time_avg = 0;
    if (msg_queue_cnt > 0)
        msg_queue_delay_time_avg = msg_queue_delay_time / msg_queue_cnt;
    if (msg_batch_cnt > 0)
    {
        mbuf_send_intv_time_avg = mbuf_send_intv_time / msg_batch_cnt;
        msg_batch_size_msgs_avg = msg_batch_size_msgs / msg_batch_cnt;
        msg_batch_size_bytes_avg = msg_batch_size_bytes / msg_batch_cnt;
    }
    if (msg_recv_cnt > 0)
    {
        msg_recv_time_avg = msg_recv_time / msg_recv_cnt;
        msg_unpack_time_avg = msg_unpack_time / msg_recv_cnt;
    }
    if (msg_send_cnt > 0)
    {
        msg_send_time_avg = msg_send_time / msg_send_cnt;
    }
    fprintf(outf,
            ",msg_queue_delay_time=%f"
            ",msg_queue_cnt=%ld"
            ",msg_queue_enq_cnt=%ld"
            ",msg_queue_delay_time_avg=%f"
            ",msg_send_time=%f"
            ",msg_send_time_avg=%f"
            ",msg_recv_time=%f"
            ",msg_recv_time_avg=%f"
            ",msg_recv_idle_time=%f"
            ",msg_batch_cnt=%ld"
            ",msg_batch_size_msgs=%ld"
            ",msg_batch_size_msgs_avg=%f"
            ",msg_batch_size_bytes=%ld"
            ",msg_batch_size_bytes_avg=%f"
            ",msg_batch_size_bytes_to_server=%ld"
            ",msg_batch_size_bytes_to_client=%ld"
            ",msg_send_cnt=%ld"
            ",msg_recv_cnt=%ld"
            ",msg_unpack_time=%f"
            ",msg_unpack_time_avg=%f"
            ",mbuf_send_intv_time=%f"
            ",mbuf_send_intv_time_avg=%f"
            ",msg_copy_output_time=%f",
            msg_queue_delay_time / BILLION, msg_queue_cnt, msg_queue_enq_cnt, msg_queue_delay_time_avg / BILLION, msg_send_time / BILLION, msg_send_time_avg / BILLION, msg_recv_time / BILLION, msg_recv_time_avg / BILLION, msg_recv_idle_time / BILLION, msg_batch_cnt, msg_batch_size_msgs, msg_batch_size_msgs_avg, msg_batch_size_bytes, msg_batch_size_bytes_avg, msg_batch_size_bytes_to_server, msg_batch_size_bytes_to_client, msg_send_cnt, msg_recv_cnt, msg_unpack_time / BILLION, msg_unpack_time_avg / BILLION, mbuf_send_intv_time / BILLION, mbuf_send_intv_time_avg / BILLION, msg_copy_output_time / BILLION);

#if TIME_PROF_ENABLE
    for (uint64_t i = 0; i < (g_rem_thread_cnt + g_send_thread_cnt); ++i)
    {
        fprintf(outf, ",io_thd_idle_time_%lu=%f", i, io_thread_idle_time[i] / BILLION);
    }
#endif

    if (!prog)
    {
        client_client_latency.quicksort(0, client_client_latency.cnt - 1);
        fprintf(outf,
                ",ccl0=%f"
                ",ccl1=%f"
                ",ccl10=%f"
                ",ccl25=%f"
                ",ccl50=%f"
                ",ccl75=%f"
                ",ccl90=%f"
                ",ccl95=%f"
                ",ccl96=%f"
                ",ccl97=%f"
                ",ccl98=%f"
                ",ccl99=%f"
                ",ccl100=%f",
                (double)client_client_latency.get_idx(0) / BILLION, (double)client_client_latency.get_percentile(1) / BILLION, (double)client_client_latency.get_percentile(10) / BILLION, (double)client_client_latency.get_percentile(25) / BILLION, (double)client_client_latency.get_percentile(50) / BILLION, (double)client_client_latency.get_percentile(75) / BILLION, (double)client_client_latency.get_percentile(90) / BILLION, (double)client_client_latency.get_percentile(95) / BILLION, (double)client_client_latency.get_percentile(96) / BILLION, (double)client_client_latency.get_percentile(97) / BILLION, (double)client_client_latency.get_percentile(98) / BILLION, (double)client_client_latency.get_percentile(99) / BILLION, (double)client_client_latency.get_idx(client_client_latency.cnt - 1) / BILLION);
    }

    //client_client_latency.print(outf);
}

void Stats_thd::combine(Stats_thd *stats)
{
    if (stats->total_runtime > total_runtime)
        total_runtime = stats->total_runtime;

    //last_start_commit_latency.append(stats->first_start_commit_latency);
    //first_start_commit_latency.append(stats->first_start_commit_latency);
    //start_abort_commit_latency.append(stats->start_abort_commit_latency);
    client_client_latency.append(stats->client_client_latency);
    // Execution
    txn_cnt += stats->txn_cnt;
    remote_txn_cnt += stats->remote_txn_cnt;
    local_txn_cnt += stats->local_txn_cnt;
    local_txn_start_cnt += stats->local_txn_start_cnt;
    total_txn_commit_cnt += stats->total_txn_commit_cnt;
    local_txn_commit_cnt += stats->local_txn_commit_cnt;
    remote_txn_commit_cnt += stats->remote_txn_commit_cnt;
    total_txn_abort_cnt += stats->total_txn_abort_cnt;
    unique_txn_abort_cnt += stats->unique_txn_abort_cnt;
    local_txn_abort_cnt += stats->local_txn_abort_cnt;
    remote_txn_abort_cnt += stats->remote_txn_abort_cnt;
    txn_run_time += stats->txn_run_time;
    multi_part_txn_cnt += stats->multi_part_txn_cnt;
    multi_part_txn_run_time += stats->multi_part_txn_run_time;
    single_part_txn_cnt += stats->single_part_txn_cnt;
    single_part_txn_run_time += stats->single_part_txn_run_time;
    txn_write_cnt += stats->txn_write_cnt;
    record_write_cnt += stats->record_write_cnt;
    parts_touched += stats->parts_touched;

    // Breakdown
    ts_alloc_time += stats->ts_alloc_time;
    abort_time += stats->abort_time;
    txn_manager_time += stats->txn_manager_time;
    txn_index_time += stats->txn_index_time;
    txn_validate_time += stats->txn_validate_time;
    txn_cleanup_time += stats->txn_cleanup_time;

#if CONSENSUS == PBFT || CONSENSUS == DBFT
    time_pre_prepare += stats->time_pre_prepare;
    time_prepare += stats->time_prepare;
    time_commit += stats->time_commit;
    time_execute += stats->time_execute;
    tput_msg += stats->tput_msg;
    msg_cl_in += stats->msg_cl_in;
    msg_node_in += stats->msg_node_in;
    msg_cl_out += stats->msg_cl_out;
    msg_node_out += stats->msg_node_out;
#endif

    // Transaction stats
    txn_total_process_time += stats->txn_total_process_time;
    txn_process_time += stats->txn_process_time;
    txn_total_local_wait_time += stats->txn_total_local_wait_time;
    txn_local_wait_time += stats->txn_local_wait_time;
    txn_total_remote_wait_time += stats->txn_total_remote_wait_time;
    txn_remote_wait_time += stats->txn_remote_wait_time;
    //txn_total_twopc_time+=stats->txn_total_twopc_time;
    //txn_twopc_time+=stats->txn_twopc_time;

    // Client
    txn_sent_cnt += stats->txn_sent_cnt;
    cl_send_intv += stats->cl_send_intv;

    // Work queue
    work_queue_wait_time += stats->work_queue_wait_time;
    work_queue_cnt += stats->work_queue_cnt;
    work_queue_enq_cnt += stats->work_queue_enq_cnt;
    work_queue_mtx_wait_time += stats->work_queue_mtx_wait_time;
    work_queue_new_cnt += stats->work_queue_new_cnt;
    work_queue_new_wait_time += stats->work_queue_new_wait_time;
    work_queue_old_cnt += stats->work_queue_old_cnt;
    work_queue_old_wait_time += stats->work_queue_old_wait_time;
    work_queue_enqueue_time += stats->work_queue_enqueue_time;
    work_queue_dequeue_time += stats->work_queue_dequeue_time;
    work_queue_conflict_cnt += stats->work_queue_conflict_cnt;

    // Worker thread
    worker_idle_time += stats->worker_idle_time;
    worker_activate_txn_time += stats->worker_activate_txn_time;
    worker_deactivate_txn_time += stats->worker_deactivate_txn_time;
    worker_release_msg_time += stats->worker_release_msg_time;
    worker_process_time += stats->worker_process_time;
    worker_process_cnt += stats->worker_process_cnt;
    for (uint64_t i = 0; i < NO_MSG; i++)
    {
        worker_process_cnt_by_type[i] += stats->worker_process_cnt_by_type[i];
        worker_process_time_by_type[i] += stats->worker_process_time_by_type[i];
    }

    // IO
    msg_queue_delay_time += stats->msg_queue_delay_time;
    msg_queue_cnt += stats->msg_queue_cnt;
    msg_queue_enq_cnt += stats->msg_queue_enq_cnt;
    msg_send_time += stats->msg_send_time;
    msg_recv_time += stats->msg_recv_time;
    msg_recv_idle_time += stats->msg_recv_idle_time;
    msg_batch_cnt += stats->msg_batch_cnt;
    msg_batch_size_msgs += stats->msg_batch_size_msgs;
    msg_batch_size_bytes += stats->msg_batch_size_bytes;
    msg_batch_size_bytes_to_server += stats->msg_batch_size_bytes_to_server;
    msg_batch_size_bytes_to_client += stats->msg_batch_size_bytes_to_client;
    msg_send_cnt += stats->msg_send_cnt;
    msg_recv_cnt += stats->msg_recv_cnt;
    msg_unpack_time += stats->msg_unpack_time;
    mbuf_send_intv_time += stats->mbuf_send_intv_time;
    msg_copy_output_time += stats->msg_copy_output_time;

#if TIME_PROF_ENABLE
    for (uint64_t i = 0; i < (g_rem_thread_cnt + g_send_thread_cnt); ++i)
    {
        io_thread_idle_time[i] += stats->io_thread_idle_time[i];
    }
#endif

    // Concurrency control, general
    //cc_conflict_cnt+=stats->cc_conflict_cnt;
    //txn_wait_cnt+=stats->txn_wait_cnt;
    //txn_conflict_cnt+=stats->txn_conflict_cnt;

    // Transaction Table
    txn_table_new_cnt += stats->txn_table_new_cnt;
    txn_table_get_cnt += stats->txn_table_get_cnt;
    txn_table_release_cnt += stats->txn_table_release_cnt;
    txn_table_cflt_cnt += stats->txn_table_cflt_cnt;
    txn_table_cflt_size += stats->txn_table_cflt_size;
    txn_table_get_time += stats->txn_table_get_time;
    txn_table_release_time += stats->txn_table_release_time;
    txn_table_min_ts_time += stats->txn_table_min_ts_time;

    //for(uint64_t i = 0; i < 40; i ++) {
    //  mtx[i]+=stats->mtx[i];
    //}

    // Latency

    lat_work_queue_time += stats->lat_work_queue_time;
    lat_msg_queue_time += stats->lat_msg_queue_time;
    lat_cc_block_time += stats->lat_cc_block_time;
    lat_cc_time += stats->lat_cc_time;
    lat_process_time += stats->lat_process_time;
    lat_abort_time += stats->lat_abort_time;
    lat_network_time += stats->lat_network_time;
    lat_other_time += stats->lat_other_time;

    lat_l_loc_work_queue_time += stats->lat_l_loc_work_queue_time;
    lat_l_loc_msg_queue_time += stats->lat_l_loc_msg_queue_time;
    lat_l_loc_cc_block_time += stats->lat_l_loc_cc_block_time;
    lat_l_loc_cc_time += stats->lat_l_loc_cc_time;
    lat_l_loc_process_time += stats->lat_l_loc_process_time;
    lat_l_loc_abort_time += stats->lat_l_loc_abort_time;

    lat_short_work_queue_time += stats->lat_short_work_queue_time;
    lat_short_msg_queue_time += stats->lat_short_msg_queue_time;
    lat_short_cc_block_time += stats->lat_short_cc_block_time;
    lat_short_cc_time += stats->lat_short_cc_time;
    lat_short_process_time += stats->lat_short_process_time;
    lat_short_network_time += stats->lat_short_network_time;
    lat_short_batch_time += stats->lat_short_batch_time;

    lat_s_loc_work_queue_time += stats->lat_s_loc_work_queue_time;
    lat_s_loc_msg_queue_time += stats->lat_s_loc_msg_queue_time;
    lat_s_loc_cc_block_time += stats->lat_s_loc_cc_block_time;
    lat_s_loc_cc_time += stats->lat_s_loc_cc_time;
    lat_s_loc_process_time += stats->lat_s_loc_process_time;

    lat_l_rem_work_queue_time += stats->lat_l_rem_work_queue_time;
    lat_l_rem_msg_queue_time += stats->lat_l_rem_msg_queue_time;
    lat_l_rem_cc_block_time += stats->lat_l_rem_cc_block_time;
    lat_l_rem_cc_time += stats->lat_l_rem_cc_time;
    lat_l_rem_process_time += stats->lat_l_rem_process_time;

    lat_s_rem_work_queue_time += stats->lat_s_rem_work_queue_time;
    lat_s_rem_msg_queue_time += stats->lat_s_rem_msg_queue_time;
    lat_s_rem_cc_block_time += stats->lat_s_rem_cc_block_time;
    lat_s_rem_cc_time += stats->lat_s_rem_cc_time;
    lat_s_rem_process_time += stats->lat_s_rem_process_time;
}

void Stats::init(uint64_t thread_cnt)
{
    if (!STATS_ENABLE)
        return;

    thd_cnt = thread_cnt;
    _stats = new Stats_thd *[thread_cnt];
    totals = new Stats_thd;

    for (uint64_t i = 0; i < thread_cnt; i++)
    {
        _stats[i] = (Stats_thd *)
                        mem_allocator.align_alloc(sizeof(Stats_thd));
        _stats[i]->init(i);
        _stats[i]->clear();
    }

    totals->init(0);
    totals->clear();
}

void Stats::free()
{
    if (!STATS_ENABLE)
        return;

    for (uint64_t i = 0; i < thd_cnt; i++)
    {
        _stats[i]->free();
        mem_allocator.free(_stats[i], 0);
    }

    delete[] _stats;
    totals->free();
    delete totals;
}

void Stats::clear(uint64_t tid)
{
}

void Stats::print_client(bool prog)
{
    fflush(stdout);
    if (!STATS_ENABLE)
        return;

    totals->clear();
    for (uint64_t i = 0; i < thd_cnt; i++)
        totals->combine(_stats[i]);

    FILE *outf;
    //if (output_file != NULL)
    //  outf = fopen(output_file, "w");
    //else
    outf = stdout;
    if (prog)
        fprintf(outf, "[prog] ");
    else
        fprintf(outf, "[summary] ");
    totals->print_client(outf, prog);
    mem_util(outf);
    cpu_util(outf);

    if (prog)
    {
        fprintf(outf, "\n");
        //for (uint32_t k = 0; k < g_node_id; ++k) {
        for (uint32_t k = 0; k < g_servers_per_client; ++k)
        {
            printf("tif_node%u=%d, ", k, client_man.get_inflight(k));
        }
        printf("\n");
    }
    else
    {
    }
    fflush(stdout);
}

void Stats::print(bool prog)
{

    fflush(stdout);
    if (!STATS_ENABLE)
        return;

    totals->clear();
    for (uint64_t i = 0; i < thd_cnt; i++)
        totals->combine(_stats[i]);

    // An exception.
    for (uint64_t i = 0; i < g_thread_cnt; i++)
    {
        idle_worker_times[i] = _stats[i]->worker_idle_time;
    }
    //totals->worker_idle_time = _stats[3]->worker_idle_time;

    FILE *outf;
    //if (output_file != NULL)
    //  outf = fopen(output_file, "w");
    //else
    outf = stdout;
    if (prog)
        fprintf(outf, "[prog] ");
    else
        fprintf(outf, "[summary] ");
    totals->print(outf, prog);
    mem_util(outf);
    cpu_util(outf);

    fprintf(outf, "\n");
    fflush(outf);
    if (!prog)
    {
        //print_cnts(outf);
        //print_lat_distr();
    }
    fprintf(outf, "\n");
    fflush(outf);
    //if (output_file != NULL) {
    //      fclose(outf);
    //}
}

uint64_t Stats::get_txn_cnts()
{
    if (!STATS_ENABLE || g_node_id >= g_node_cnt)
        return 0;
    uint64_t limit = g_thread_cnt + g_rem_thread_cnt;
    uint64_t total_txn_cnt = 0;
    for (uint64_t tid = 0; tid < limit; tid++)
    {
        total_txn_cnt += _stats[tid]->txn_cnt;
    }
    //printf("total_txn_cnt: %lu\n",total_txn_cnt);
    return total_txn_cnt;
}

void Stats::print_lat_distr()
{
#if PRT_LAT_DISTR
    printf("\n[all_lat] ");
    uint64_t limit = 0;
    if (g_node_id < g_node_cnt)
        limit = g_thread_cnt;
    else
        limit = g_client_thread_cnt;
    for (UInt32 tid = 0; tid < limit; tid++)
        _stats[tid]->all_lat.print(stdout);
#endif
}

void Stats::print_lat_distr(uint64_t min, uint64_t max)
{
#if PRT_LAT_DISTR
    printf("\n[all_lat] ");
    _stats[0]->all_lat.print(stdout, min, max);
#endif
}

void Stats::util_init()
{
    struct tms timeSample;
    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;
}

void Stats::print_util()
{
}

int Stats::parseLine(char *line)
{
    int i = strlen(line);
    while (*line < '0' || *line > '9')
        line++;
    line[i - 3] = '\0';
    i = atoi(line);
    return i;
}

void Stats::mem_util(FILE *outf)
{
    FILE *file = fopen("/proc/self/status", "r");
    int result = -1;
    char line[128];

    // Physical memory used by current process, in KB
    while (fgets(line, 128, file) != NULL)
    {
        if (strncmp(line, "VmRSS:", 6) == 0)
        {
            result = parseLine(line);
            fprintf(outf,
                    "phys_mem_usage=%d\n", result);
        }
        if (strncmp(line, "VmSize:", 7) == 0)
        {
            result = parseLine(line);
            fprintf(outf,
                    "virt_mem_usage=%d\n", result);
        }
    }
    fclose(file);
}

void Stats::cpu_util(FILE *outf)
{
    clock_t now;
    struct tms timeSample;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU)
    {
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else
    {
        percent = (timeSample.tms_stime - lastSysCPU) +
                  (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        if (ISSERVER)
        {
            percent /= (g_total_thread_cnt); //numProcessors;
        }
        else if (ISCLIENT)
        {
            percent /= (g_total_client_thread_cnt); //numProcessors;
        }
        percent *= 100;
    }
    fprintf(outf,
            ",cpu_ttl=%f", percent);
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;
}

void Stats_thd::print(FILE *outf, bool prog)
{

    string node_id = to_string(g_node_id +1);
    string filename = "toInflux_R";
    filename.append(node_id);
    filename.append("_.out");
    filename="./monitor/"+filename;

    ofstream file;
    file.open(filename.c_str(), ios_base::out | ios_base::in);  // will not create file
    if (!file.is_open())
    {    
        file.clear();
        file.open(filename.c_str(), std::ofstream::app);  // will create if necessary
        //file << ip_add;
        file << "tput\n";
    } 
    else
    {
        file.close();
        file.open (filename.c_str(),std::ofstream::app);
    }

    fprintf(outf, "filename: %s", filename.c_str());

    fprintf(outf,
            "\ntotal_runtime=%f\n", total_runtime / BILLION);
    // Execution
    double tput = 0;
    // double txn_run_avg_time = 0;
    // double multi_part_txn_avg_time = 0;
    // double single_part_txn_avg_time = 0;
    // double avg_parts_touched = 0;
    if (total_runtime > 0)
        tput = txn_cnt / (total_runtime / BILLION);
    // if(txn_cnt > 0) {
    //   txn_run_avg_time = txn_run_time / txn_cnt;
    //   avg_parts_touched = ((double)parts_touched) / txn_cnt;
    // }

    file << tput;
    file << "\n";
    file.close(); 
    
    fprintf(outf, "\ntput=%f\ntxn_cnt=%ld\n", tput, txn_cnt);

    double work_queue_wait_avg_time = 0;
    // double work_queue_mtx_wait_avg = 0;
    // double work_queue_new_wait_avg_time = 0;
    // double work_queue_old_wait_avg_time = 0;
    if (work_queue_cnt > 0)
    {
        work_queue_wait_avg_time = work_queue_wait_time / work_queue_cnt;
        // work_queue_mtx_wait_avg = work_queue_mtx_wait_time / work_queue_cnt;
    }
    // if(work_queue_new_cnt > 0)
    //   work_queue_new_wait_avg_time = work_queue_new_wait_time / work_queue_new_cnt;
    // if(work_queue_old_cnt > 0)
    //   work_queue_old_wait_avg_time = work_queue_old_wait_time / work_queue_old_cnt;
    // Work queue
    fprintf(outf,
            ",work_queue_wait_time=%f\n"
            ",work_queue_cnt=%ld\n"
            ",work_queue_enq_cnt=%ld\n"
            ",work_queue_wait_avg_time=%f\n"
            ",work_queue_enqueue_time=%f\n"
            ",work_queue_dequeue_time=%f\n",
            work_queue_wait_time / BILLION, work_queue_cnt, work_queue_enq_cnt, work_queue_wait_avg_time / BILLION, work_queue_enqueue_time / BILLION, work_queue_dequeue_time / BILLION);
    // Worker thread
    // double worker_process_avg_time = 0;
    // if(worker_process_cnt > 0)
    //   worker_process_avg_time = worker_process_time / worker_process_cnt;
    fprintf(outf,
            ",worker_idle_time=%f\n"
            ",worker_release_msg_time=%f\n",
            worker_idle_time / BILLION, worker_release_msg_time / BILLION);

    // Displays idle time for each worker thread.
    for (uint64_t i = 0; i < g_thread_cnt; i++)
    {
        fprintf(outf,
                "idle_time_worker %ld=%f\n", i, idle_worker_times[i] / BILLION);
    }

    // IO
    double msg_send_time_avg = 0;
    double msg_recv_time_avg = 0;
    // double msg_queue_delay_time_avg = 0;
    // if(msg_queue_cnt > 0)
    //   msg_queue_delay_time_avg = msg_queue_delay_time / msg_queue_cnt;

    if (msg_recv_cnt > 0)
    {
        msg_recv_time_avg = msg_recv_time / msg_recv_cnt;
    }
    if (msg_send_cnt > 0)
    {
        msg_send_time_avg = msg_send_time / msg_send_cnt;
    }
    fprintf(outf,
            // "msg_queue_delay_time=%f\n"
            // "msg_queue_cnt=%ld\n"
            // "msg_queue_enq_cnt=%ld\n"
            // "msg_queue_delay_time_avg=%f\n"
            "msg_send_time=%f\n"
            "msg_send_time_avg=%f\n"
            "msg_recv_time=%f\n"
            "msg_recv_time_avg=%f\n"
            "msg_recv_idle_time=%f\n"
            "msg_send_cnt=%ld\n"
            "msg_recv_cnt=%ld\n"
            // ,msg_queue_delay_time / BILLION
            // ,msg_queue_cnt
            // ,msg_queue_enq_cnt
            // ,msg_queue_delay_time_avg / BILLION
            ,
            msg_send_time / BILLION, msg_send_time_avg / BILLION, msg_recv_time / BILLION, msg_recv_time_avg / BILLION, msg_recv_idle_time / BILLION, msg_send_cnt, msg_recv_cnt);

    fprintf(outf,
            "bytes_received=%ld\n"
            "bytes_sent=%ld\n",
            bytes_received, bytes_sent);

    fprintf(outf,
            "msg_size_client_batch=%ld\n"
            "msg_size_batch_req=%ld\n"
#if CONSENSUS == PBFT
            "msg_size_commit=%ld\n"
            "msg_size_prepare=%ld\n"
#endif
            "msg_size_checkpoint=%ld\n"
            "msg_size_client_response=%ld\n"
#if GBFT
            "msg_size_ccm=%ld\n"
#endif
#if CONSENSUS == HOTSTUFF
            "msg_size_vote=%ld\n"
#endif
#if STEWARD
            "acc_cert_msg_size=%ld\n"
#endif
            ,
            client_batch_msg_size, batch_req_msg_size
#if CONSENSUS == PBFT
            ,
            commit_msg_size, prepare_msg_size
#endif
            ,
            checkpoint_msg_size, client_response_msg_size
#if GBFT
            ,
            ccm_msg_size
#endif
#if CONSENSUS == HOTSTUFF
            ,
            vote_msg_size
#endif
#if STEWARD
            ,
            acc_cert_msg_size
#endif
    );

#if TIME_PROF_ENABLE
    for (uint64_t i = 0; i < (g_rem_thread_cnt + g_send_thread_cnt); ++i)
    {
        fprintf(outf, ",io_thd_idle_time_%lu=%f", i, io_thread_idle_time[i] / BILLION);
    }
#endif

    // Transaction Table
    double txn_table_get_avg_time = 0;
    if (txn_table_get_cnt > 0)
        txn_table_get_avg_time = txn_table_get_time / txn_table_get_cnt;
    double txn_table_release_avg_time = 0;
    if (txn_table_release_cnt > 0)
        txn_table_release_avg_time = txn_table_release_time / txn_table_release_cnt;
    fprintf(outf,
            "txn_table_new_cnt=%ld\n"
            "txn_table_get_cnt=%ld\n"
            "txn_table_release_cnt=%ld\n"
            "txn_table_cflt_cnt=%ld\n"
            "txn_table_cflt_size=%ld\n"
            "txn_table_get_time=%f\n"
            "txn_table_release_time=%f\n"
            "txn_table_min_ts_time=%f\n"
            "txn_table_get_avg_time=%f\n"
            "txn_table_release_avg_time=%f\n"
            // Transaction Table
            ,
            txn_table_new_cnt, txn_table_get_cnt, txn_table_release_cnt, txn_table_cflt_cnt, txn_table_cflt_size, txn_table_get_time / BILLION, txn_table_release_time / BILLION, txn_table_min_ts_time / BILLION, txn_table_get_avg_time / BILLION, txn_table_release_avg_time / BILLION);
}

void Stats_thd::set_message_size(uint64_t rtype, uint64_t size)
{
    switch (rtype)
    {
    case CL_RSP:
        this->client_response_msg_size = size;
        break;
#if CLIENT_BATCH
    case CL_BATCH:
#else
    case CL_QRY:
#endif
        this->client_batch_msg_size = size;
        break;
#if BATCH_ENABLE == BSET
    case BATCH_REQ:
#else
    case PBFT_PRE:
#endif
        this->batch_req_msg_size = size;
        break;
    case PBFT_CHKPT_MSG:
        this->checkpoint_msg_size = size;
        break;
#if CONSENSUS == PBFT
    case PBFT_COMMIT_MSG:
        this->commit_msg_size = size;
        break;
    case PBFT_PREP_MSG:
        this->prepare_msg_size = size;
        break;
#endif
#if GBFT
    case COMMIT_CERTIFICATE_MSG:
        this->ccm_msg_size = size;
        break;
#endif
#if STEWARD
    case STW_ACC_CERT:
        this->acc_cert_msg_size = size;
        break;
#endif
#if CONSENSUS == HOTSTUFF
    case PREP_VOTE: // 27
    case PRE_COMMIT:
    case COMMIT_VOTE:
    case COMMIT:
    case DECIDE_VOTE:
    case DECIDE:
    case SEND_NEXT_VIEW:
    case NEXT_VIEW:
        this->vote_msg_size = size;
        break;
#endif
    default:
        break;
    }
}
