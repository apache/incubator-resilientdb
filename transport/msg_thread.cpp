#include "msg_thread.h"
#include "msg_queue.h"
#include "message.h"
#include "mem_alloc.h"
#include "transport.h"
#include "query.h"
#include "ycsb_query.h"
#include "pool.h"
#include "global.h"
#include "crypto.h"

void MessageThread::init(uint64_t thd_id)
{
    buffer_cnt = g_total_node_cnt;
    DEBUG_M("MessageThread::init buffer[] alloc\n");
    buffer = (mbuf **)mem_allocator.align_alloc(sizeof(mbuf *) * buffer_cnt);
    for (uint64_t n = 0; n < buffer_cnt; n++)
    {
        DEBUG_M("MessageThread::init mbuf alloc\n");
        buffer[n] = (mbuf *)mem_allocator.align_alloc(sizeof(mbuf));
        buffer[n]->init(n);
        buffer[n]->reset(n);
    }
    _thd_id = thd_id;
}

void MessageThread::check_and_send_batches()
{
    for (uint64_t dest_node_id = 0; dest_node_id < buffer_cnt; dest_node_id++)
    {
        if (buffer[dest_node_id]->ready())
        {
            send_batch(dest_node_id);
        }
    }
}

void MessageThread::send_batch(uint64_t dest_node_id)
{
    mbuf *sbuf = buffer[dest_node_id];
    assert(sbuf->cnt > 0);
    ((uint32_t *)sbuf->buffer)[2] = sbuf->cnt;

    DEBUG("Send batch of %ld msgs to %ld\n", sbuf->cnt, dest_node_id);
    tport_man.send_msg(_thd_id, dest_node_id, sbuf->buffer, sbuf->ptr);
    sbuf->reset(dest_node_id);
}

void MessageThread::run()
{
    Message *msg = NULL;
    uint64_t dest_node_id;
    vector<uint64_t> dest;
    vector<string> allsign;
    mbuf *sbuf;

    // Relative Id of the server's output thread.
    UInt32 td_id = _thd_id % g_this_send_thread_cnt;

    dest = msg_queue.dequeue(get_thd_id(), allsign, msg);

    if (!msg)
    {
        check_and_send_batches();
        if (idle_starttime == 0)
        {
            idle_starttime = get_sys_clock();
        }
        return;
    }
    if (idle_starttime > 0 && simulation->is_warmup_done())
    {
        output_thd_idle_time[td_id] += get_sys_clock() - idle_starttime;
        idle_starttime = 0;
    }
    assert(msg);

    for (uint64_t i = 0; i < dest.size(); i++)
    {
        dest_node_id = dest[i];

        if (ISSERVER)
        {
            if (dest_node_id % g_this_send_thread_cnt != td_id)
            {
                continue;
            }
        }

        // Adding signature, if present.
        if (allsign.size() > 0)
        {
            msg->signature = allsign[i];
            switch (msg->rtype)
            {
            case CL_BATCH:
                msg->pubKey = getOtherRequiredKey(dest_node_id);
                break;
#if GBFT
            case GBFT_COMMIT_CERTIFICATE_MSG:
                if (((GeoBFTCommitCertificateMessage *)msg)->forwarding_from == (uint64_t)-1)
                    msg->pubKey = getOtherRequiredKey(dest_node_id);
                break;
#endif
            default:
                msg->pubKey = getCmacRequiredKey(dest_node_id);
            }
            msg->sigSize = msg->signature.size();
            msg->keySize = msg->pubKey.size();
        }

        sbuf = buffer[dest_node_id];
        if (!sbuf->fits(msg->get_size()))
        {
            assert(sbuf->cnt > 0);
            cout << "not fitting " << sbuf->cnt << endl;
            send_batch(dest_node_id);
        }
#if VIEW_CHANGES
        if (msg->rtype == VIEW_CHANGE)
            sbuf->force = true;
        if (msg->rtype == NEW_VIEW)
            sbuf->force = true;
#endif
        if (msg->rtype == PBFT_CHKPT_MSG)
            sbuf->force = true;
        msg->copy_to_buf(&(sbuf->buffer[sbuf->ptr]));

        sbuf->cnt += 1;
        sbuf->ptr += msg->get_size();

        if (sbuf->starttime == 0)
            sbuf->starttime = get_sys_clock();

        check_and_send_batches();
    }
    Message::release_message(msg);
}
