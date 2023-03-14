#ifndef _IOTHREAD_H_
#define _IOTHREAD_H_

#include "global.h"
#include "message.h"

class Workload;
class MessageThread;

class InputThread : public Thread
{
public:
    RC run();
    RC client_recv_loop();
    RC server_recv_loop();
    void check_for_init_done();
    void setup();
    void managekey(KeyExchange *keyex);

#if TIME_PROF_ENABLE
    uint64_t io_thd_id;
#endif
};

class OutputThread : public Thread
{
public:
    RC run();
    void setup();
    MessageThread *messager;
};

#endif
