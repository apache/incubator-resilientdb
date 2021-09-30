#ifndef _THREAD_H_
#define _THREAD_H_

#include "global.h"

class Workload;

class Thread
{
public:
    virtual ~Thread() {}
    void send_init_done_to_all_nodes();
    void progress_stats();
    void heartbeat();
    uint64_t _thd_id;
    uint64_t _node_id;
    Workload *_wl;
    myrand rdm;
    uint64_t run_starttime;

    uint64_t get_thd_id();
    uint64_t get_node_id();
    void tsetup();
    
    // TODO for experimental purpose: force one view change
    bool has_view_changed();

    void init(uint64_t thd_id, uint64_t node_id, Workload *workload);
    // the following function must be in the form void* (*)(void*)
    // to run with pthread.
    // conversion is done within the function.
    virtual RC run() = 0;
    virtual void setup() = 0;

private:
    uint64_t prog_time;
    uint64_t heartbeat_time;
};

#endif
