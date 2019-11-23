#include "global.h"
#include "thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "math.h"
#include "helper.h"
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

            vector<string> emptyvec;
            vector<uint64_t> dest;
            dest.push_back(i);
            msg_queue.enqueue(get_thd_id(), Message::create_message(INIT_DONE), emptyvec, dest);
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

void Thread::progress_stats()
{
    if (get_thd_id() == 0)
    {
#if TIME_ENABLE
        uint64_t now_time = get_sys_clock();
#else
        uint64_t now_time = get_wall_clock();
#endif
        if (now_time - prog_time >= g_prog_timer)
        {
            prog_time = now_time;
            SET_STATS(get_thd_id(), total_runtime, prog_time - simulation->run_starttime);

            if (ISCLIENT)
            {
                stats.print_client(true);
            }
            else
            {
                stats.print(true);
            }
        }
    }
}
