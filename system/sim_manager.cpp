#include "global.h"
#include "sim_manager.h"

void SimManager::init()
{
    sim_done = false;
    warmup = false;
    warmup_end_time = 0;
    start_set = false;
    sim_init_done = false;
    txn_cnt = 0;
    inflight_cnt = 0;
    epoch_txn_cnt = 0;
    worker_epoch = 1;
    seq_epoch = 0;
    rsp_cnt = g_total_node_cnt - 1;

#if TIME_ENABLE
    run_starttime = get_sys_clock();
#else
    run_starttime = get_wall_clock();
#endif
    last_worker_epoch_time = run_starttime;
    last_seq_epoch_time = get_wall_clock();
}

void SimManager::set_starttime(uint64_t starttime)
{
    if (ATOM_CAS(start_set, false, true))
    {
        run_starttime = starttime;
        last_worker_epoch_time = starttime;
        sim_done = false;
        printf("Starttime set to %ld\n", run_starttime);
    }
}

bool SimManager::timeout()
{
#if TIME_ENABLE
    return (get_sys_clock() - run_starttime) >= g_done_timer + g_warmup_timer;
#else
    return (get_wall_clock() - run_starttime) >= g_done_timer + g_warmup_timer;
#endif
}

bool SimManager::is_done()
{
    bool done = sim_done || timeout();
    if (done && !sim_done)
    {
        set_done();
    }
    return done;
}

bool SimManager::is_warmup_done()
{
    if (warmup)
        return true;
    bool done = ((get_sys_clock() - run_starttime) >= g_warmup_timer);
    if (done)
    {
        ATOM_CAS(warmup_end_time, 0, get_sys_clock());
        ATOM_CAS(warmup, false, true);
    }
    return done;
}
bool SimManager::is_setup_done()
{
    return sim_init_done;
}

void SimManager::set_setup_done()
{
    ATOM_CAS(sim_init_done, false, true);
}

void SimManager::set_done()
{
    if (ATOM_CAS(sim_done, false, true))
    {
        if (warmup_end_time == 0)
            warmup_end_time = run_starttime;
        SET_STATS(0, total_runtime, get_sys_clock() - warmup_end_time);
    }
}

void SimManager::process_setup_msg()
{
    uint64_t rsp_left = ATOM_SUB_FETCH(rsp_cnt, 1);
    if (rsp_left == 0)
    {
        set_setup_done();
    }
}

void SimManager::inc_txn_cnt()
{
    ATOM_ADD(txn_cnt, 1);
}

void SimManager::inc_inflight_cnt()
{
    ATOM_ADD(inflight_cnt, 1);
}

void SimManager::dec_inflight_cnt()
{
    ATOM_SUB(inflight_cnt, 1);
}

void SimManager::inc_epoch_txn_cnt()
{
    ATOM_ADD(epoch_txn_cnt, 1);
}

void SimManager::decr_epoch_txn_cnt()
{
    ATOM_SUB(epoch_txn_cnt, 1);
}

uint64_t SimManager::get_seq_epoch()
{
    return seq_epoch;
}

void SimManager::advance_seq_epoch()
{
    ATOM_ADD(seq_epoch, 1);
    last_seq_epoch_time += g_seq_batch_time_limit;
}

uint64_t SimManager::get_worker_epoch()
{
    return worker_epoch;
}

void SimManager::next_worker_epoch()
{
    last_worker_epoch_time = get_sys_clock();
    ATOM_ADD(worker_epoch, 1);
}

double SimManager::seconds_from_start(uint64_t time)
{
    return (double)(time - run_starttime) / BILLION;
}
