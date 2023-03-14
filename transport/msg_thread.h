#ifndef _MSG_THREAD_H_
#define _MSG_THREAD_H_

#include "global.h"
#include "nn.hpp"

struct mbuf
{
    char *buffer;
    uint64_t starttime;
    uint64_t ptr;
    uint64_t cnt;
    bool wait;
    uint64_t dest_node_id;
    bool force = false;

    void init(uint64_t dest_id)
    {
        buffer = (char *)nng_alloc(g_msg_size);
        dest_node_id = dest_id;
    }
    void reset(uint64_t dest_id)
    {
        starttime = 0;
        cnt = 0;
        wait = false;
        ((uint32_t *)buffer)[0] = dest_id;
        ((uint32_t *)buffer)[1] = g_node_id;
        ptr = sizeof(uint32_t) * 3;
        dest_node_id = dest_id;
    }
    void copy(char *p, uint64_t s)
    {
        assert(ptr + s <= g_msg_size);
        if (cnt == 0)
            starttime = get_sys_clock();
        COPY_BUF_SIZE(buffer, p, ptr, s);
    }
    bool fits(uint64_t s)
    {
        return (ptr + s) <= g_msg_size;
    }
    bool ready()
    {
        if (simulation->is_warmup_done() && ISSERVER)
        {
            if (cnt == MESSAGE_PER_BUFFER || (force && cnt)){
                force = false;
                return true;

            }
            return false;
        }
        else
        {
            if (cnt == 0)
                return false;
            if ((get_sys_clock() - starttime) >= g_msg_time_limit)
                return true;
            return false;
        }
    }
};

class MessageThread
{
public:
    void init(uint64_t thd_id);
    void run();
    void check_and_send_batches();
    void send_batch(uint64_t dest_node_id);
    void copy_to_buffer(mbuf *sbuf, RemReqType type, BaseQuery *qry);
    uint64_t get_msg_size(RemReqType type, BaseQuery *qry);
    void rack(mbuf *sbuf, BaseQuery *qry);
    void rprepare(mbuf *sbuf, BaseQuery *qry);
    void rfin(mbuf *sbuf, BaseQuery *qry);
    void cl_rsp(mbuf *sbuf, BaseQuery *qry);
    void log_msg(mbuf *sbuf, BaseQuery *qry);
    void log_msg_rsp(mbuf *sbuf, BaseQuery *qry);
    void rinit(mbuf *sbuf, BaseQuery *qry);
    void rqry(mbuf *sbuf, BaseQuery *qry);
    void rfwd(mbuf *sbuf, BaseQuery *qry);
    void rdone(mbuf *sbuf, BaseQuery *qry);
    void rqry_rsp(mbuf *sbuf, BaseQuery *qry);
    void rtxn(mbuf *sbuf, BaseQuery *qry);
    void rtxn_seq(mbuf *sbuf, BaseQuery *qry);
    uint64_t get_thd_id() { return _thd_id; }
    uint64_t idle_starttime = 0;

private:
    mbuf **buffer;
    uint64_t buffer_cnt;
    uint64_t _thd_id;
};

#endif
