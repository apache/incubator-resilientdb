#include "global.h"
#include "thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "math.h"
#include "msg_queue.h"
#include "message.h"

void Thread::heartbeat()
{
}

void Thread::send_init_done_to_all_nodes()
{
    for (uint64_t i = 0; i < g_total_node_cnt; i++)
    {
        if (i != g_node_id)
        {
            printf("Send INIT_DONE to %ld\n", i);

            vector<uint64_t> dest;
            dest.push_back(i);
            msg_queue.enqueue(get_thd_id(), Message::create_message(INIT_DONE), dest);
            dest.clear();
        }
    }
}

void Thread::init(uint64_t thd_id, uint64_t node_id, Workload *workload)
{
    _thd_id = thd_id;
    _node_id = node_id;
    _wl = workload;
    rdm.init(_thd_id);
}

uint64_t Thread::get_thd_id() { return _thd_id; }
uint64_t Thread::get_node_id() { return _node_id; }

void Thread::tsetup()
{
    printf("Setup %ld:%ld\n", _node_id, _thd_id);
    fflush(stdout);
    pthread_barrier_wait(&warmup_bar);

    setup();

    printf("Running %ld:%ld\n", _node_id, _thd_id);
    fflush(stdout);
    pthread_barrier_wait(&warmup_bar);

#if TIME_ENABLE
    run_starttime = get_sys_clock();
#else
    run_starttime = get_wall_clock();
#endif
    simulation->set_starttime(run_starttime);
    prog_time = run_starttime;
    heartbeat_time = run_starttime;
    pthread_barrier_wait(&warmup_bar);
}

bool Thread::has_view_changed()
{
    if (get_current_view(get_thd_id()))
        return true;
    return false;
}

void Thread::progress_stats()
{
#if TIME_ENABLE
    uint64_t now_time = get_sys_clock();
#else
    uint64_t now_time = get_wall_clock();
#endif

    if (now_time - prog_time >= g_prog_timer)
    {
        prog_time = now_time;
        if (get_thd_id() == 0)
        {
            SET_STATS(get_thd_id(), total_runtime, prog_time - simulation->warmup_end_time);

            if (ISCLIENT)
            {
                stats.print_client(true);
            }
            else
            {
                stats.print(true);
                uint64_t exec_thread = g_worker_thread_cnt + g_batching_thread_cnt + g_checkpointing_thread_cnt + g_execution_thread_cnt - 1;
                SET_STATS(exec_thread, previous_interval_cross_shard_txn_cnt, stats._stats[exec_thread]->cross_shard_txn_cnt);
                SET_STATS(exec_thread, previous_interval_txn_cnt, stats._stats[exec_thread]->txn_cnt);
            }
        }
    }
}
