#include "global.h"
#include "helper.h"
#include "thread.h"
#include "io_thread.h"
#include "query.h"
#include "ycsb_query.h"
#include "mem_alloc.h"
#include "transport.h"
#include "math.h"
#include "msg_thread.h"
#include "msg_queue.h"
#include "message.h"
#include "client_txn.h"
#include "work_queue.h"
#include "timer.h"
//#include "crypto.h"

void InputThread::managekey(KeyExchange *keyex)
{
    string algorithm = keyex->pkey.substr(0, 4);
    keyex->pkey = keyex->pkey.substr(4, keyex->pkey.size() - 4);
    cout << "Algo: " << algorithm << " :: " << keyex->return_node << endl;
    cout << "Storing the key: " << keyex->pkey << " ::size: " << keyex->pkey.size() << endl;
    fflush(stdout);

#if CRYPTO_METHOD_CMAC_AES
    if (algorithm == "CMA-")
    {
        cmacOthersKeys[keyex->return_node] = keyex->pkey;
        //CMACrecv[keyex->return_node] = CMACgenerateInstance(keyex->pkey);
    }
#endif

    // When using ED25519 we create the verifier for this pkey.
    // This saves some time during the verification
#if CRYPTO_METHOD_ED25519
    if (algorithm == "ED2-")
    {
        g_pub_keys[keyex->return_node] = keyex->pkey;
        byte byteKey[CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH];
        copyStringToByte(byteKey, keyex->pkey);
        verifier[keyex->return_node] = CryptoPP::ed25519::Verifier(byteKey);
    }
#elif CRYPTO_METHOD_RSA
    if (algorithm == "RSA-")
    {
        g_pub_keys[keyex->return_node] = keyex->pkey;
    }
#endif
}

void InputThread::setup()
{

    // Increment commonVar.
    batchMTX.lock();
    commonVar++;
    batchMTX.unlock();

#if TIME_PROF_ENABLE
    io_thd_id = _thd_id - g_thread_cnt;
#endif

    std::vector<Message *> *msgs;
    while (!simulation->is_setup_done())
    {
        msgs = tport_man.recv_msg(get_thd_id());
        if (msgs == NULL)
            continue;

        while (!msgs->empty())
        {
            Message *msg = msgs->front();

            if (msg->rtype == INIT_DONE)
            {
                printf("Received INIT_DONE from node %ld\n", msg->return_node_id);
                fflush(stdout);
                simulation->process_setup_msg();
            }
            else
            {
                if (!ISSERVER)
                {
                    // Storing the keys.
                    if (msg->rtype == KEYEX)
                    {
                        KeyExchange *keyex = (KeyExchange *)msg;
                        managekey(keyex);
                    }
                    else if (msg->rtype == READY)
                    {
                        totKey++;
                        if (totKey == g_node_cnt)
                        {
                            keyMTX.lock();
                            keyAvail = true;
                            keyMTX.unlock();
                        }
                    }
                }
                else
                {
                    assert(ISSERVER || ISREPLICA);
                    //printf("Received Msg %d from node %ld\n",msg->rtype,msg->return_node_id);

                    // Linearizing requests.
                    if (msg->rtype == CL_BATCH)
                    {
                        msg->txn_id = get_and_inc_next_idx();
                    }

                    work_queue.enqueue(get_thd_id(), msg, false);
                }
            }
            msgs->erase(msgs->begin());
        }
        delete msgs;
    }
}

RC InputThread::run()
{
    tsetup();
    printf("Running InputThread %ld\n", _thd_id);

    if (ISCLIENT)
    {
        client_recv_loop();
    }
    else
    {
        server_recv_loop();
    }

    return FINISH;
}

RC InputThread::client_recv_loop()
{
    int rsp_cnts[g_node_cnt];
    memset(rsp_cnts, 0, g_node_cnt * sizeof(int));

    run_starttime = get_sys_clock();
    uint64_t return_node_offset;
    uint64_t inf;
#if TIME_PROF_ENABLE
    uint64_t idle_starttime = 0;
#endif
    std::vector<Message *> *msgs;

    double sumlat = 0;
    uint64_t txncmplt = 0;

    while (!simulation->is_done())
    {
        heartbeat();
        msgs = tport_man.recv_msg(get_thd_id());
        if (msgs == NULL)
        {

#if TIME_PROF_ENABLE
            if (idle_starttime == 0)
            {
                idle_starttime = get_sys_clock();
            }
#endif
            continue;
        }

#if TIME_PROF_ENABLE
        if (idle_starttime > 0)
        {
            INC_STATS(io_thd_id, io_thread_idle_time, get_sys_clock() - idle_starttime);
            idle_starttime = 0;
        }
#endif

        while (!msgs->empty())
        {
            Message *msg = msgs->front();

            // Initial message processing, prior to actual consensus.
            if (msg->rtype == KEYEX)
            {
                KeyExchange *keyex = (KeyExchange *)msg;
                managekey(keyex);
                msgs->erase(msgs->begin());
                continue;
            }
            else if (msg->rtype == READY)
            {
                totKey++;
                if (totKey == g_node_cnt)
                {

                    keyMTX.lock();
                    keyAvail = true;
                    keyMTX.unlock();
                }
                msgs->erase(msgs->begin());
                continue;
            }

            //cout<<"Node: "<<msg->return_node_id <<" :: Txn: "<< msg->txn_id <<"\n";
            //fflush(stdout);

            return_node_offset = get_view();
            if (msg->rtype != CL_RSP)
            {
                cout << "Mtype: " << msg->rtype << " :: Nd: " << msg->return_node_id << "\n";
                fflush(stdout);
                assert(0);
            }
            ClientResponseMessage *clrsp = (ClientResponseMessage *)msg;
            // Check if the response is valid.
            if (clrsp->validate())
            {

#if TIMER_ON
                // End the timer.
                client_timer->endTimer(clrsp->client_ts[get_batch_size() - 1]);
#endif
                //cout << "validated: " << clrsp->txn_id << "\n";
                //fflush(stdout);

#if VIEW_CHANGES
                //cout << "View: " << clrsp->view << "\n";
                //fflush(stdout);

                // This should happen only once after the view change.
                if (get_view() != clrsp->view)
                {
                    // Extract the number of pending requests.
                    uint64_t pending = client_man.get_inflight(get_view());
                    for (uint64_t j = 0; j < pending; j++)
                    {
                        client_man.inc_inflight(clrsp->view);
                    }

                    // Move to new view.
                    set_view(clrsp->view);
                    return_node_offset = get_view();
                }
#endif

#if CLIENT_RESPONSE_BATCH == true
                for (uint64_t k = 0; k < g_batch_size; k++)
                {
                    rsp_cnts[return_node_offset]++;
                    INC_STATS(get_thd_id(), txn_cnt, 1);

                    uint64_t timespan = get_sys_clock() - clrsp->client_ts[k];
                    INC_STATS(get_thd_id(), txn_run_time, timespan);
                    if (warmup_done)
                    {
                        INC_STATS_ARR(get_thd_id(), client_client_latency, timespan);
                    }

                    sumlat = sumlat + timespan;
                    txncmplt++;
                    inf = client_man.dec_inflight(return_node_offset);
                }
#else // !CLIENT_RESPONSE_BATCH

                rsp_cnts[return_node_offset]++;
                INC_STATS(get_thd_id(), txn_cnt, 1);
                uint64_t timespan = get_sys_clock() - clrsp->client_startts;
                INC_STATS(get_thd_id(), txn_run_time, timespan);
                if (warmup_done)
                {
                    INC_STATS_ARR(get_thd_id(), client_client_latency, timespan);
                }

                sumlat = sumlat + timespan;
                txncmplt++;

                inf = client_man.dec_inflight(return_node_offset);

#endif // CLIENT_RESPONSE_BATCH
                assert(inf >= 0);
            }
            // delete message here
            msgs->erase(msgs->begin());
        }
        delete msgs;
    }

    printf("AVG: %f\n", (sumlat / (txncmplt * BILLION)));
    fflush(stdout);
    printf("TXNCOMP: %ld\n", txncmplt);
    fflush(stdout);

    printf("FINISH %ld:%ld\n", _node_id, _thd_id);
    fflush(stdout);
    return FINISH;
}

RC InputThread::server_recv_loop()
{

    myrand rdm;
    rdm.init(get_thd_id());
    RC rc = RCOK;
    assert(rc == RCOK);
    uint64_t starttime = 0;
    uint64_t idle_starttime = 0;
    std::vector<Message *> *msgs;
    while (!simulation->is_done())
    {
        heartbeat();

#if VIEW_CHANGES
        if (g_node_id != get_current_view(get_thd_id()))
        {
            uint64_t tid = get_thd_id() - 1;
            uint32_t nchange = get_newView(tid);

            if (nchange)
            {
                set_current_view(get_thd_id(), get_current_view(get_thd_id()) + 1);
                set_newView(tid, false);
            }
        }
#endif

        msgs = tport_man.recv_msg(get_thd_id());

        if (msgs == NULL)
        {
            if (idle_starttime == 0)
            {
                idle_starttime = get_sys_clock();
            }
            continue;
        }
        if (idle_starttime > 0)
        {
            starttime += get_sys_clock() - idle_starttime;
            idle_starttime = 0;
        }

        while (!msgs->empty())
        {
            Message *msg = msgs->front();
            if (msg->rtype == INIT_DONE)
            {
                msgs->erase(msgs->begin());
                continue;
            }
            if (msg->rtype == CL_BATCH)
            {
                // Linearizing requests.
                msg->txn_id = get_and_inc_next_idx();
                INC_STATS(_thd_id, msg_cl_in, 1);
            }

            work_queue.enqueue(get_thd_id(), msg, false);
            msgs->erase(msgs->begin());
        }
        delete msgs;
    }
    cout << "Input: " << _thd_id << " :: " << (starttime * 1.0) / BILLION << "\n";
    fflush(stdout);
    return FINISH;
}

void OutputThread::setup()
{
    DEBUG_Q("OutputThread::setup MessageThread alloc %lu\n", _thd_id);
    messager = (MessageThread *)mem_allocator.alloc(sizeof(MessageThread));
    uint64_t io_thd_id = _thd_id;
#if TIME_PROF_ENABLE
    io_thd_id = _thd_id - g_thread_cnt;
    if (ISCLIENT)
    {
        assert(_thd_id >= (g_client_thread_cnt));
    }
    else
    {
        assert(_thd_id >= (g_thread_cnt));
    }
#endif

    messager->init(io_thd_id);

    // Increment commonVar.
    batchMTX.lock();
    commonVar++;
    batchMTX.unlock();
    messager->idle_starttime = 0;
    while (!simulation->is_setup_done())
    {
        messager->run();
    }
}

RC OutputThread::run()
{

    tsetup();
    printf("Running OutputThread %ld\n", _thd_id);

    while (!simulation->is_done())
    {
        heartbeat();
        messager->run();
    }

    printf("Output %ld: %f\n", _thd_id, output_thd_idle_time[_thd_id % g_send_thread_cnt] / BILLION);
    fflush(stdout);
    return FINISH;
}
