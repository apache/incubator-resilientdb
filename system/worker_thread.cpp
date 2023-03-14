#include "global.h"
#include "message.h"
#include "thread.h"
#include "worker_thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "ycsb_query.h"
#include "math.h"
#include "msg_thread.h"
#include "msg_queue.h"
#include "work_queue.h"
#include "message.h"
#include "timer.h"
#include "chain.h"

void WorkerThread::send_key()
{
    // Send everyone the public key.
    for (uint64_t i = 0; i < g_node_cnt + g_client_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
#if CRYPTO_METHOD_RSA || CRYPTO_METHOD_ED25519
        Message *msg = Message::create_message(KEYEX);
        KeyExchange *keyex = (KeyExchange *)msg;
        // The four first letters of the message set the type
#if CRYPTO_METHOD_RSA
        keyex->pkey = "RSA-" + g_public_key;
        cout << "Sending RSA: " << g_public_key << endl;
#elif CRYPTO_METHOD_ED25519
        keyex->pkey = "ED2-" + g_public_key;
        cout << "Sending ED25519: " << g_public_key << endl;
#endif
        fflush(stdout);

        keyex->pkeySz = keyex->pkey.size();
        keyex->return_node = g_node_id;
        // msg_queue.enqueue(get_thd_id(), keyex, i);

        vector<uint64_t> dest;
        dest.push_back(i);
        msg_queue.enqueue(get_thd_id(), keyex, dest);
        dest.clear();

#endif

#if CRYPTO_METHOD_CMAC_AES
        Message *msgCMAC = Message::create_message(KEYEX);
        KeyExchange *keyexCMAC = (KeyExchange *)msgCMAC;
        keyexCMAC->pkey = "CMA-" + cmacPrivateKeys[i];
        keyexCMAC->pkeySz = keyexCMAC->pkey.size();
        keyexCMAC->return_node = g_node_id;
        // msg_queue.enqueue(get_thd_id(), keyexCMAC, i);

        msg_queue.enqueue(get_thd_id(), keyexCMAC, dest);
        dest.clear();

        cout << "Sending CMAC " << cmacPrivateKeys[i] << endl;
        fflush(stdout);
#endif
    }
}
void WorkerThread::unset_ready_txn(TxnManager *tman)
{
    // uint64_t spin_wait_starttime = 0;
    while (true)
    {
        bool ready = tman->unset_ready();
        if (!ready)
        {
            // if (spin_wait_starttime == 0)
            //     spin_wait_starttime = get_sys_clock();
            continue;
        }
        else
        {
            // if (spin_wait_starttime > 0)
            //     INC_STATS(_thd_id, worker_spin_wait_time, get_sys_clock() - spin_wait_starttime);
            break;
        }
    }
}
void WorkerThread::setup()
{
    // Increment commonVar.
    batchMTX.lock();
    commonVar++;
    batchMTX.unlock();

    if (get_thd_id() == 0)
    {
        while (commonVar < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt)
            ;

        send_init_done_to_all_nodes();

        send_key();
    }
    _thd_txn_id = 0;
}

void WorkerThread::process(Message *msg)
{
    RC rc __attribute__((unused));

    switch (msg->get_rtype())
    {
    case KEYEX:
        rc = process_key_exchange(msg);
        break;
    case CL_BATCH:
        rc = process_client_batch(msg);
        break;
    case BATCH_REQ:
        rc = process_batch(msg);
        break;
    case PBFT_CHKPT_MSG:
        rc = process_pbft_chkpt_msg(msg);
        break;
    case EXECUTE_MSG:
        rc = process_execute_msg(msg);
        break;
#if VIEW_CHANGES
    case VIEW_CHANGE:
        rc = process_view_change_msg(msg);
        break;
    case NEW_VIEW:
        rc = process_new_view_msg(msg);
        break;
#endif
    case PBFT_PREP_MSG:
        rc = process_pbft_prep_msg(msg);
        break;
    case PBFT_COMMIT_MSG:
        rc = process_pbft_commit_msg(msg);
        break;
#if GBFT
    case GBFT_COMMIT_CERTIFICATE_MSG:
        rc = process_gbft_commit_certificate_msg(msg);
        break;
#endif
#if MIN_PBFT_ALL_TO_ALL
    case PREP_CERTIFICATE_MSG:
        rc = process_prep_certificate_msg(msg);
        break;
#endif
    default:
        printf("rtype: %d from %ld\n", msg->get_rtype(), msg->return_node_id);
        fflush(stdout);
        assert(false);
        break;
    }
}

RC WorkerThread::process_key_exchange(Message *msg)
{
    KeyExchange *keyex = (KeyExchange *)msg;

    string algorithm = keyex->pkey.substr(0, 4);
    keyex->pkey = keyex->pkey.substr(4, keyex->pkey.size() - 4);
    // cout << "Algo: " << algorithm << " :: " <<keyex->return_node << endl;
    // cout << "Storing the key: " << keyex->pkey << " ::size: " << keyex->pkey.size() << endl;
    fflush(stdout);

#if CRYPTO_METHOD_CMAC_AES
    if (algorithm == "CMA-")
    {
        cmacOthersKeys[keyex->return_node] = keyex->pkey;
        receivedKeys[keyex->return_node]--;
    }
#endif

// When using ED25519 we create the verifier for this pkey.
// This saves some time during the verification
#if CRYPTO_METHOD_ED25519
    if (algorithm == "ED2-")
    {
        // cout << "Key length: " << keyex->pkey.size() << " for ED255: " << CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH << endl;
        g_pub_keys[keyex->return_node] = keyex->pkey;
        byte byteKey[CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH];
        copyStringToByte(byteKey, keyex->pkey);
        verifier[keyex->return_node] = CryptoPP::ed25519::Verifier(byteKey);
        receivedKeys[keyex->return_node]--;
    }

#elif CRYPTO_METHOD_RSA
    if (algorithm == "RSA-")
    {
        g_pub_keys[keyex->return_node] = keyex->pkey;
        receivedKeys[keyex->return_node]--;
    }
#endif

    bool sendReady = true;
    // Check if we have the keys of every node
    uint64_t totnodes = g_node_cnt + g_client_node_cnt;
    for (uint64_t i = 0; i < totnodes; i++)
    {
        if (receivedKeys[i] != 0)
        {
            sendReady = false;
        }
    }

    if (sendReady)
    {
        // Send READY to clients.
        for (uint64_t i = g_node_cnt; i < totnodes; i++)
        {
            Message *rdy = Message::create_message(READY);
            // msg_queue.enqueue(get_thd_id(), rdy, i);

            vector<uint64_t> dest;
            dest.push_back(i);
            msg_queue.enqueue(get_thd_id(), rdy, dest);
            dest.clear();
        }

    }

    return RCOK;
}

void WorkerThread::release_txn_man(uint64_t txn_id, uint64_t batch_id)
{
    txn_table.release_transaction_manager(get_thd_id(), txn_id, batch_id);
}

TxnManager *WorkerThread::get_transaction_manager(uint64_t txn_id, uint64_t batch_id)
{
    TxnManager *tman = txn_table.get_transaction_manager(get_thd_id(), txn_id, batch_id);
    return tman;
}

/* Returns the id for the next txn. */
uint64_t WorkerThread::get_next_txn_id()
{
    uint64_t txn_id = get_batch_size() * next_set;
    return txn_id;
}

#if LOCAL_FAULT
/* This function focibly fails non-primary replicas. */
void WorkerThread::fail_nonprimary()
{
    if (g_node_id > g_min_invalid_nodes)
    {
        if (g_node_id - num_nodes_to_fail <= g_min_invalid_nodes)
        {
            if (get_sys_clock() - simulation->run_starttime < 10 * BILLION)
                return;

            for (uint64_t j = 1; j <= num_nodes_to_fail; j++)
            {
                uint64_t fnode = g_min_invalid_nodes + j;
                for (uint i = 0; i < g_send_thread_cnt; i++)
                {
                    stopMTX[i].lock();
                    stop_nodes[i].push_back(fnode);
                    stopMTX[i].unlock();
                }
            }
            cout << "trying to terminate\n";
            assert(0);
        }
    }
}

#endif

#if TIMER_ON
void WorkerThread::add_timer(Message *msg, string qryhash)
{
    // TODO if condition is for experimental purpose: force one view change
    if (this->has_view_changed())
        return;

    char *tbuf = create_msg_buffer(msg);
    Message *deepMsg = deep_copy_msg(tbuf, msg);
    deepMsg->return_node_id = msg->return_node_id;
    server_timer->startTimer(qryhash, deepMsg);
    delete_msg_buffer(tbuf);
}

void WorkerThread::remove_timer(string qryhash)
{
    // TODO if condition is for experimental purpose: force one view change
    if (this->has_view_changed())
        return;

    server_timer->endTimer(qryhash);
}
#endif

#if VIEW_CHANGES
/*
Each non-primary replica continuously checks the timer for each batch.
If there is a timeout then it initiates the view change.
This requires sending a view change message to each replica.
*/
void WorkerThread::check_for_timeout()
{
    // TODO if condition is for experimental purpose: force one view change
    if (this->has_view_changed())
        return;

    if (g_node_id != get_current_view(get_thd_id()) && server_timer->checkTimer())
    {
        // Pause the timer to avoid causing further view changes.
        server_timer->pauseTimer();

        // cout << "Begin Changing View" << endl;
        fflush(stdout);

        Message *msg = Message::create_message(VIEW_CHANGE);
        TxnManager *local_tman = get_transaction_manager(get_curr_chkpt(), 0);
        // cout << "Initializing" << endl;
        fflush(stdout);

        ((ViewChangeMsg *)msg)->init(get_thd_id(), local_tman);

        // cout << "Going to send" << endl;
        fflush(stdout);

        // send view change messages
        vector<uint64_t> dest;
        for (uint64_t i = 0; i < g_node_cnt; i++)
        {
            // avoid sending msg to old primary
            if (i == get_current_view(get_thd_id()))
            {
                continue;
            }
            else if (i == g_node_id)
            {
                continue;
            }
            dest.push_back(i);
        }

        char *buf = create_msg_buffer(msg);
        Message *deepCMsg = deep_copy_msg(buf, msg);
        // Send to other replicas.
        msg_queue.enqueue(get_thd_id(), deepCMsg, dest);
        dest.clear();

        // process a message for itself
        deepCMsg = deep_copy_msg(buf, msg);
        work_queue.enqueue(get_thd_id(), deepCMsg, false);

        delete_msg_buffer(buf);
        Message::release_message(msg); // Releasing the message.

        fflush(stdout);
    }
}

/* This function causes the forced failure of the primary replica at a
desired time. */
void WorkerThread::fail_primary(Message *msg, uint64_t time)
{
    if (!simulation->is_warmup_done())
        return;
    uint64_t elapsesd_time = get_sys_clock() - simulation->warmup_end_time;
    if (g_node_id == 0 && elapsesd_time > time)
    // if (g_node_id == 0 && msg->txn_id > 9)
    {
        uint64_t count = 0;
        while (true)
        {
            count++;
            if (count > 1000000000)
            {
                assert(0);
            }
        }
    }
}

void WorkerThread::store_batch_msg(BatchRequests *breq)
{
    char *bbuf = create_msg_buffer(breq);
    Message *deepCMsg = deep_copy_msg(bbuf, breq);
    storeBatch((BatchRequests *)deepCMsg);
    delete_msg_buffer(bbuf);
}

/*
The client forwarded its request to a non-primary replica.
This maybe a potential case for a malicious primary.
Hence store the request, start timer and forward it to the primary replica.
*/
void WorkerThread::client_query_check(ClientQueryBatch *clbtch)
{
    // TODO if condition is for experimental purpose: force one view change
    if (this->has_view_changed())
        return;

    // cout << "REQUEST: " << clbtch->return_node_id << "\n";
    // fflush(stdout);

    // start timer when client broadcasts an unexecuted message
    //  Last request of the batch.
    YCSBClientQueryMessage *qry = clbtch->cqrySet[clbtch->batch_size - 1];
    add_timer(clbtch, calculateHash(qry->getString()));

    // Forward to the primary.
    vector<uint64_t> dest;
    dest.push_back(get_current_view(get_thd_id()));

    char *tbuf = create_msg_buffer(clbtch);
    Message *deepCMsg = deep_copy_msg(tbuf, clbtch);
    msg_queue.enqueue(get_thd_id(), deepCMsg, dest);
    dest.clear();
    delete_msg_buffer(tbuf);
}

/****************************************/
/* Functions for handling view changes. */
/****************************************/

RC WorkerThread::process_view_change_msg(Message *msg)
{
    cout << "PROCESS VIEW CHANGE from" << msg->return_node_id << "\n";
    fflush(stdout);

    ViewChangeMsg *vmsg = (ViewChangeMsg *)msg;

    // Ignore the old view messages or those delivered after view change.
    if (vmsg->view <= get_current_view(get_thd_id()))
    {
        return RCOK;
    }

    // assert view is correct
    assert(vmsg->view == ((get_current_view(get_thd_id()) + 1) % g_node_cnt));

    // cout << "validating view change message" << endl;
    // fflush(stdout);

    if (!vmsg->addAndValidate(get_thd_id()))
    {
        // cout << "waiting for more view change messages" << endl;
        return RCOK;
    }

    // cout << "executing view change message" << endl;
    // fflush(stdout);

    // Only new primary performs rest of the actions.
    if (g_node_id == vmsg->view)
    {
        // cout << "New primary rules!" << endl;
        // fflush(stdout);

        // Move to next view
        uint64_t total_thds = g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt;
        for (uint64_t i = 0; i < total_thds; i++)
        {
            set_view(i, vmsg->view);
        }

        Message *newViewMsg = Message::create_message(NEW_VIEW);
        NewViewMsg *nvmsg = (NewViewMsg *)newViewMsg;
        nvmsg->init(get_thd_id());

        cout << "New primary is ready to fire" << endl;
        fflush(stdout);

        // Adding older primary to list of failed nodes.
        for (uint i = 0; i < g_send_thread_cnt; i++)
        {
            stopMTX[i].lock();
            stop_nodes[i].push_back(g_node_id - 1);
            stopMTX[i].unlock();
        }

        // send new view messages
        vector<uint64_t> dest;
        for (uint64_t i = 0; i < g_node_cnt; i++)
        {
            if (i == g_node_id)
            {
                continue;
            }
            dest.push_back(i);
        }

        char *buf = create_msg_buffer(nvmsg);
        Message *deepCMsg = deep_copy_msg(buf, nvmsg);
        msg_queue.enqueue(get_thd_id(), deepCMsg, dest);
        dest.clear();

        delete_msg_buffer(buf);
        Message::release_message(newViewMsg);

        // Remove all the ViewChangeMsgs.
        clearAllVCMsg();

        // Setting up the next txn id.
        set_next_idx(curr_next_index() / get_batch_size());

        set_expectedExecuteCount(curr_next_index() + get_batch_size() - 2);
        cout << "expectedExeCount = " << expectedExecuteCount << endl;
        fflush(stdout);

        // Start the re-directed requests.
        Timer *tmap;
        Message *retrieved_msg;
        for (uint64_t i = 0; i < server_timer->timerSize(); i++)
        {
            tmap = server_timer->fetchPendingRequests(i);
            retrieved_msg = tmap->get_msg();
            // YCSBClientQueryMessage *yc = ((ClientQueryBatch *)msg)->cqrySet[0];

            // cout << "MSG: " << yc->return_node << " :: Key: " << yc->requests[0]->key << "\n";
            // fflush(stdout);

            char *buf = create_msg_buffer(retrieved_msg);
            Message *deepCMsg = deep_copy_msg(buf, retrieved_msg);
            deepCMsg->return_node_id = retrieved_msg->return_node_id;

            // Assigning an identifier to the batch.
            deepCMsg->txn_id = get_and_inc_next_idx();
            work_queue.enqueue(get_thd_id(), deepCMsg, false);
            delete_msg_buffer(buf);
        }

        // Clear the timer.
        server_timer->removeAllTimers();
    }

    return RCOK;
}

RC WorkerThread::process_new_view_msg(Message *msg)
{
    cout << "PROCESS NEW VIEW " << msg->txn_id << "\n";
    fflush(stdout);

    NewViewMsg *nvmsg = (NewViewMsg *)msg;
    if (!nvmsg->validate(get_thd_id()))
    {
        assert(0);
        return RCOK;
    }

    // Adding older primary to list of failed nodes.
    for (uint i = 0; i < g_send_thread_cnt; i++)
    {
        stopMTX[i].lock();
        stop_nodes[i].push_back(nvmsg->view - 1);
        stopMTX[i].unlock();
    }

    // Move to the next view.
    uint64_t total_thds = g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt;
    for (uint64_t i = 0; i < total_thds; i++)
    {
        set_view(i, nvmsg->view);
    }

    // cout << "new primary changed view" << endl;
    // fflush(stdout);

    // Remove all the ViewChangeMsgs.
    clearAllVCMsg();

    // Setting up the next txn id.
    set_next_idx((curr_next_index() + 1) % get_batch_size());

    set_expectedExecuteCount(curr_next_index() + get_batch_size() - 2);

    // Clear the timer entries.
    server_timer->removeAllTimers();

    // Restart the timer.
    server_timer->resumeTimer();

    return RCOK;
}

#endif // VIEW_CHANGES

/**
 * Starting point for each worker thread.
 *
 * Each worker-thread created in the main() starts here. Each worker-thread is alive
 * till the time simulation is not done, and continuousy perform a set of tasks.
 * Thess tasks involve, dequeuing a message from its queue and then processing it
 * through call to the relevant function.
 */
RC WorkerThread::run()
{
    tsetup();
    printf("Running WorkerThread %ld\n", _thd_id);

    uint64_t agCount = 0, ready_starttime, idle_starttime = 0;

    // Setting batch (only relevant for batching threads).
    next_set = 0;

    while (!simulation->is_done())
    {
        txn_man = NULL;
        heartbeat();
        progress_stats();

#if VIEW_CHANGES
        // Thread 0 continously monitors the timer for each batch.
        if (get_thd_id() == 0)
        {
            check_for_timeout();
        }

#endif

        // Dequeue a message from its work_queue.
        Message *msg = work_queue.dequeue(get_thd_id());
        if (!msg)
        {
            if (idle_starttime == 0)
                idle_starttime = get_sys_clock();
            continue;
        }
        if (idle_starttime > 0)
        {
            INC_STATS(_thd_id, worker_idle_time, get_sys_clock() - idle_starttime);
            idle_starttime = 0;
        }

        // Remove redundant messages.
        if (exception_msg_handling(msg))
        {
            continue;
        }

        // Based on the type of the message, we try acquiring the transaction manager.
        if (msg->rtype != BATCH_REQ && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG)
        {
            txn_man = get_transaction_manager(msg->txn_id, 0);

            ready_starttime = get_sys_clock();
            bool ready = txn_man->unset_ready();
            if (!ready)
            {
                // cout << "Placing: Txn: " << msg->txn_id << " Type: " << msg->rtype << "\n";
                // fflush(stdout);
                //  Return to work queue, end processing
                work_queue.enqueue(get_thd_id(), msg, true);
                continue;
            }
            txn_man->register_thread(this);
        }

        // Th execut-thread only picks up the next batch for execution.
        if (msg->rtype == EXECUTE_MSG)
        {
            if (msg->txn_id != get_expectedExecuteCount())
            {
                // Return to work queue.
                agCount++;
                work_queue.enqueue(get_thd_id(), msg, true);
                continue;
            }
        }

        if (!simulation->is_warmup_done())
        {
            stats.set_message_size(msg->rtype, msg->get_size());
        }
        process(msg);
        ready_starttime = get_sys_clock();
        if (txn_man)
        {
            bool ready = txn_man->set_ready();
            if (!ready)
            {
                cout << "FAIL: " << txn_man->get_txn_id() << " :: RT: " << msg->rtype << "\n";
                fflush(stdout);
                assert(ready);
            }
        }

        // delete message
        ready_starttime = get_sys_clock();
        Message::release_message(msg);

        INC_STATS(get_thd_id(), worker_release_msg_time, get_sys_clock() - ready_starttime);
    }
    printf("FINISH: %ld\n", agCount);
    fflush(stdout);

    return FINISH;
}

RC WorkerThread::init_phase()
{
    RC rc = RCOK;
    return rc;
}

bool WorkerThread::is_cc_new_timestamp()
{
    return false;
}

#if BANKING_SMART_CONTRACT
/**
 * This function sets up the required fields of the txn manager.
 *
 * @param clqry One Client Transaction (or Query).
 */
void WorkerThread::init_txn_man(BankingSmartContractMessage *bsc)
{
    txn_man->client_id = bsc->return_node_id;
    txn_man->client_startts = bsc->client_startts;
    SmartContract *smart_contract;
    switch (bsc->type)
    {
    case BSC_TRANSFER:
    {
        TransferMoneySmartContract *tm = new TransferMoneySmartContract();
        tm->amount = bsc->inputs[1];
        tm->source_id = bsc->inputs[0];
        tm->dest_id = bsc->inputs[2];
        tm->type = BSC_TRANSFER;
        smart_contract = (SmartContract *)tm;
        break;
    }
    case BSC_DEPOSIT:
    {
        DepositMoneySmartContract *dm = new DepositMoneySmartContract();

        dm->amount = bsc->inputs[0];
        dm->dest_id = bsc->inputs[1];
        dm->type = BSC_DEPOSIT;
        smart_contract = (SmartContract *)dm;
        break;
    }
    case BSC_WITHDRAW:
    {
        WithdrawMoneySmartContract *wm = new WithdrawMoneySmartContract();
        wm->amount = bsc->inputs[0];
        wm->source_id = bsc->inputs[1];
        wm->type = BSC_WITHDRAW;
        smart_contract = (SmartContract *)wm;
        break;
    }
    default:
        assert(0);
        break;
    }
    txn_man->smart_contract = smart_contract;
}
#else

/**
 * This function sets up the required fields of the txn manager.
 *
 * @param clqry One Client Transaction (or Query).
 */
void WorkerThread::init_txn_man(YCSBClientQueryMessage *clqry)
{
    txn_man->client_id = clqry->return_node;
    txn_man->client_startts = clqry->client_startts;

    YCSBQuery *query = (YCSBQuery *)(txn_man->query);
    for (uint64_t i = 0; i < clqry->requests.size(); i++)
    {
        ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
        req->key = clqry->requests[i]->key;
        req->value = clqry->requests[i]->value;
        query->requests.add(req);
    }
}
#endif
/**
 * Create an message of type ExecuteMessage, to notify the execute-thread that this
 * batch of transactions are ready to be executed. This message is placed in one of the
 * several work-queues for the execute-thread.
 */
void WorkerThread::send_execute_msg()
{
    Message *tmsg = Message::create_message(txn_man, EXECUTE_MSG);
    work_queue.enqueue(get_thd_id(), tmsg, false);
}

/**
 * Execute transactions and send client response.
 *
 * This function is only accessed by the execute-thread, which executes the transactions
 * in a batch, in order. Note that the execute-thread has several queues, and at any
 * point of time, the execute-thread is aware of which is the next transaction to
 * execute. Hence, it only loops on one specific queue.
 *
 * @param msg Execute message that notifies execution of a batch.
 * @ret RC
 */
RC WorkerThread::process_execute_msg(Message *msg)
{
    // if (msg->txn_id / get_batch_size() % 100 == 0)
    // {
    //    cout << "EXECUTE " << msg->txn_id / get_batch_size() << " THREAD: " << get_thd_id() << "\n";
    //    fflush(stdout);
    // }

    uint64_t ctime = get_sys_clock();

    // This message uses txn man of index calling process_execute.
#if GBFT

    TxnManager *test = get_transaction_manager(msg->txn_id + 1, 0);
    bool local_request = is_local_request(test);
    ClientResponseMessage *crsp = 0;
    if (local_request)
    {
        Message *rsp = Message::create_message(CL_RSP);
        crsp = (ClientResponseMessage *)rsp;
        crsp->init();
    }
#else
    Message *rsp = Message::create_message(CL_RSP);
    ClientResponseMessage *crsp = (ClientResponseMessage *)rsp;
    crsp->init();
#endif

    ExecuteMessage *emsg = (ExecuteMessage *)msg;

    // Execute transactions in a shot
    uint64_t i;
    for (i = emsg->index; i < emsg->end_index - 4; i++)
    {
        // cout << "i: " << i << " :: next index: " << g_next_index << "\n";
        // fflush(stdout);

        TxnManager *tman = get_transaction_manager(i, 0);

        inc_next_index();

        // Execute the transaction
        tman->run_txn();

        // Commit the results.
        tman->commit();

        INC_STATS(get_thd_id(), txn_cnt, 1);

#if GBFT
        if (local_request)
        {
            crsp->copy_from_txn(tman);
        }
#else
        crsp->copy_from_txn(tman);
#endif
    }

    // Transactions (**95 - **98) of the batch.
    // We process these transactions separately, as we want to
    // ensure that their txn man are not held by some other thread.
    for (; i < emsg->end_index; i++)
    {
        TxnManager *tman = get_transaction_manager(i, 0);
        unset_ready_txn(tman);

        inc_next_index();

        // Execute the transaction
        tman->run_txn();

        // Commit the results.
        tman->commit();

#if GBFT
        if (local_request)
        {
            crsp->copy_from_txn(tman);
        }
#else
        crsp->copy_from_txn(tman);
#endif

        INC_STATS(get_thd_id(), txn_cnt, 1);
        // Making this txn man available.
        bool ready = tman->set_ready();
        assert(ready);
    }

    // Last Transaction of the batch.
    txn_man = get_transaction_manager(i, 0);
    unset_ready_txn(txn_man);

    inc_next_index();

    // Execute the transaction
    txn_man->run_txn();

#if ENABLE_CHAIN
    // Add the block to the blockchain.
    BlockChain->add_block(txn_man);
#endif

    // Commit the results.
    txn_man->commit();
#if GBFT
    if (local_request)
    {
        crsp->copy_from_txn(txn_man);
        vector<uint64_t> dest;
        assert(is_local_request(txn_man));
        dest.push_back(txn_man->client_id);
        msg_queue.enqueue(get_thd_id(), crsp, dest);
        dest.clear();
    }
#else
    crsp->hash = txn_man->get_hash();
    crsp->hashSize = crsp->hash.length();
    // cout << txn_man->get_txn_id() << "   " << hexStr(txn_man->get_hash().c_str(), txn_man->get_hash().length()) << endl;
    crsp->copy_from_txn(txn_man);

    vector<uint64_t> dest;
    dest.push_back(txn_man->client_id);
    msg_queue.enqueue(get_thd_id(), crsp, dest);
    dest.clear();
#endif

    INC_STATS(get_thd_id(), txn_cnt, 1);

    INC_STATS(_thd_id, tput_msg, 1);
    INC_STATS(_thd_id, msg_cl_out, 1);

    // Check and Send checkpoint messages.
    send_checkpoints(txn_man->get_txn_id());

    // Setting the next expected prepare message id.
    set_expectedExecuteCount(get_batch_size() + msg->txn_id);

    // End the execute counter.
    INC_STATS(get_thd_id(), time_execute, get_sys_clock() - ctime);
    return RCOK;
}

/**
 * This function helps in periodically sending out CheckpointMessage. At present these
 * messages are including just including information about first and last txn of the
 * batch but later we should include a digest. Further, a checkpoint is only sent after
 * transaction id is a multiple of a config.h parameter.
 *
 * @param txn_id Transaction identifier of the last transaction in the batch..
 * @ret RC
 */
void WorkerThread::send_checkpoints(uint64_t txn_id)
{
    if ((txn_id + 1) % txn_per_chkpt() == 0)
    {
        TxnManager *tman =
            txn_table.get_transaction_manager(get_thd_id(), txn_id, 0);
        tman->send_checkpoint_msgs();
    }
}

/**
 * Checkpoint and Garbage collection.
 *
 * This function waits for 2f+1 messages to mark a checkpoint. Due to different
 * processing speeds of the replicas, it is possible that a replica may receive
 * CheckpointMessage from other replicas even before it has finished executing thst
 * transaction. Hence, we need to be careful when to perform garbage collection.
 * Further, note that the CheckpointMessage messages are handled by a separate thread.
 *
 * @param msg CheckpointMessage corresponding to a checkpoint.
 * @ret RC
 */
RC WorkerThread::process_pbft_chkpt_msg(Message *msg)
{
    CheckpointMessage *ckmsg = (CheckpointMessage *)msg;
    // printf("CHKPOINT from %ld:    %ld\n", msg->return_node_id, msg->txn_id);
    // Check if message is valid.
    validate_msg(ckmsg);

    // If this checkpoint was already accessed, then return.
    if (txn_man->is_chkpt_ready())
    {
        return RCOK;
    }
    else
    {
        uint64_t num_chkpt = txn_man->decr_chkpt_cnt();
        // If sufficient number of messages received, then set the flag.
        if (num_chkpt == 0)
        {
            txn_man->set_chkpt_ready();
        }
        else
        {
            return RCOK;
        }
    }

    // Also update the next checkpoint to the identifier for this message,
    set_curr_chkpt(msg->txn_id);
    // printf("CHKPOINT Done %ld:    %ld           %ld\n", msg->return_node_id, msg->txn_id,get_curr_chkpt());

    // Now we determine what all transaction managers can we release.
    uint64_t del_range = 0;
    if (curr_next_index() > get_curr_chkpt())
    {
        del_range = get_curr_chkpt() - get_batch_size();
    }
    else
    {
        if (curr_next_index() > get_batch_size())
        {
            del_range = curr_next_index() - get_batch_size();
        }
    }

    // printf("Chkpt: %ld :: LD: %ld :: Del: %ld   Thread:%ld\n",msg->get_txn_id(), get_last_deleted_txn(), del_range, get_thd_id());
    // fflush(stdout);

    // Release Txn Managers.
    for (uint64_t i = get_last_deleted_txn(); i < del_range; i++)
    {
        release_txn_man(i, 0);
        inc_last_deleted_txn();

#if ENABLE_CHAIN
        if ((i + 1) % get_batch_size() == 0)
        {
            BlockChain->remove_block(i);
        }
#endif
    }

#if VIEW_CHANGES
    removeBatch(del_range);
#endif

    return RCOK;
}

/* Specific handling for certain messages. If no handling then return false. */
bool WorkerThread::exception_msg_handling(Message *msg)
{
    if (msg->rtype == KEYEX)
    {
        // Key Exchange message needs to pass directly.
        process(msg);
        Message::release_message(msg);
        return true;
    }

    // Release Messages that arrive after txn completion, except obviously
    // CL_BATCH as it is never late.

    if (msg->rtype != CL_BATCH)
    {
        if (msg->rtype != PBFT_CHKPT_MSG)
        {
            if (msg->txn_id <= curr_next_index())
            {
                // cout << "Extra Msg: " << msg->txn_id;
                // fflush(stdout);
                Message::release_message(msg);
                return true;
            }
        }
        else
        {
            if (msg->txn_id <= get_curr_chkpt())
            {
                Message::release_message(msg);
                return true;
            }
        }
    }

    return false;
}

/** UNUSED */
void WorkerThread::algorithm_specific_update(Message *msg, uint64_t idx)
{
    // Can update any field, if required for a different consensus protocol.
}

/**
 * This function is used by the non-primary or backup replicas to create and set
 * transaction managers for each transaction part of the BatchRequests message sent by
 * the primary replica.
 *
 * @param breq Batch of transactions as a BatchRequests message.
 * @param bid Another dimensional identifier to support more transaction managers.
 */
void WorkerThread::set_txn_man_fields(BatchRequests *breq, uint64_t bid)
{
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        txn_man = get_transaction_manager(breq->index[i], bid);

        unset_ready_txn(txn_man);

        txn_man->register_thread(this);
        txn_man->return_id = breq->return_node_id;

        // Fields that need to updated according to the specific algorithm.
        algorithm_specific_update(breq, i);

        init_txn_man(breq->requestMsg[i]);
        bool ready = txn_man->set_ready();
        assert(ready);
    }

    // We need to unset txn_man again for last txn in the batch.
    unset_ready_txn(txn_man);

    txn_man->set_hash(breq->hash);
}

/**
 * This function is used by the primary replicas to create and set
 * transaction managers for each transaction part of the ClientQueryBatch message sent
 * by the client. Further, to ensure integrity a hash of the complete batch is
 * generated, which is also used in future communication.
 *
 * @param msg Batch of transactions as a ClientQueryBatch message.
 * @param tid Identifier for the first transaction of the batch.
 */
void WorkerThread::create_and_send_batchreq(ClientQueryBatch *msg, uint64_t tid)
{
    // Creating a new BatchRequests Message.
    Message *bmsg = Message::create_message(BATCH_REQ);
    BatchRequests *breq = (BatchRequests *)bmsg;
    breq->init(get_thd_id());

#if SGX
    breq->tee_signature = msg->tee_signature;
    breq->tee_signature_size = msg->tee_signature_size;
#endif

    // Starting index for this batch of transactions.
    next_set = tid;

    // String of transactions in a batch to generate hash.
    string batchStr;

    // Allocate transaction manager for all the requests in batch.
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        uint64_t txn_id = get_next_txn_id() + i;

        // cout << "Txn: " << txn_id << " :: Thd: " << get_thd_id() << "\n";
        // fflush(stdout);
        txn_man = get_transaction_manager(txn_id, 0);

        // Unset this txn man so that no other thread can concurrently use.
        unset_ready_txn(txn_man);

        txn_man->register_thread(this);
        txn_man->return_id = msg->return_node;

        // Fields that need to updated according to the specific algorithm.
        algorithm_specific_update(msg, i);

        init_txn_man(msg->cqrySet[i]);

        // Append string representation of this txn.
        batchStr += msg->cqrySet[i]->getString();

        // Setting up data for BatchRequests Message.
        breq->copy_from_txn(txn_man, msg->cqrySet[i]);

        // Reset this txn manager.
        bool ready = txn_man->set_ready();
        assert(ready);
    }

    // Now we need to unset the txn_man again for the last txn of batch.
    unset_ready_txn(txn_man);

    // Generating the hash representing the whole batch in last txn man.
    txn_man->set_hash(calculateHash(batchStr));
    txn_man->hashSize = txn_man->hash.length();

    breq->copy_from_txn(txn_man);

    // Storing the BatchRequests message.
    txn_man->set_primarybatch(breq);

    vector<uint64_t> dest;

    // Storing all the signatures.
    for (uint64_t i = 0; i < g_node_cnt; i++)
    {
#if GBFT

        if (!is_in_same_cluster(i, g_node_id))
        {
            continue;
        }

#endif
        if (i == g_node_id)
        {
            continue;
        }
        dest.push_back(i);
    }

    msg_queue.enqueue(get_thd_id(), breq, dest);
    dest.clear();
#if FLEXI_ZYZ || MIN_ZYZ || ZYZ
    txn_man->set_committed();
    send_execute_msg();
#endif
}

/** Validates the contents of a message. */
bool WorkerThread::validate_msg(Message *msg)
{
    switch (msg->rtype)
    {
    case KEYEX:
        break;
    case CL_RSP:
        if (!((ClientResponseMessage *)msg)->validate())
        {
            assert(0);
        }
        break;

    case CL_BATCH:
        if (!((ClientQueryBatch *)msg)->validate())
        {
            assert(0);
        }
        break;
    case BATCH_REQ:
        if (!((BatchRequests *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
    case PBFT_CHKPT_MSG:
        if (!((CheckpointMessage *)msg)->validate())
        {
            assert(0);
        }
        break;
    case PBFT_PREP_MSG:
        if (!((PBFTPrepMessage *)msg)->validate())
        {
            assert(0);
        }
        break;
    case PBFT_COMMIT_MSG:
        if (!((PBFTCommitMessage *)msg)->validate())
        {
            assert(0);
        }
        break;

#if VIEW_CHANGES
    case VIEW_CHANGE:
        if (!((ViewChangeMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
    case NEW_VIEW:
        if (!((NewViewMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
#endif

    default:
        break;
    }

    return true;
}

/* Checks the hash and view of a message against current request. */
bool WorkerThread::checkMsg(Message *msg)
{
    if (msg->rtype == PBFT_PREP_MSG)
    {
        PBFTPrepMessage *pmsg = (PBFTPrepMessage *)msg;
        if ((txn_man->get_hash().compare(pmsg->hash) == 0) ||
            (get_current_view(get_thd_id()) == pmsg->view))
        {
            return true;
        }
    }
    else if (msg->rtype == PBFT_COMMIT_MSG)
    {
        PBFTCommitMessage *cmsg = (PBFTCommitMessage *)msg;
        if ((txn_man->get_hash().compare(cmsg->hash) == 0) ||
            (get_current_view(get_thd_id()) == cmsg->view))
        {
            return true;
        }
    }

    return false;
}

/**
 * Checks if the incoming PBFTPrepMessage can be accepted.
 *
 * This functions checks if the hash and view of the commit message matches that of
 * the Pre-Prepare message. Once 2f messages are received it returns a true and
 * sets the `is_prepared` flag for furtue identification.
 *
 * @param msg PBFTPrepMessage.
 * @return bool True if the transactions of this batch are prepared.
 */
bool WorkerThread::prepared(PBFTPrepMessage *msg)
{
    // cout << "Inside PREPARED: " << txn_man->get_txn_id() << "\n";
    // fflush(stdout);

    // Once prepared is set, no processing for further messages.
    if (txn_man->is_prepared())
    {
        return false;
    }

    // If BatchRequests messages has not arrived yet, then return false.
    if (txn_man->get_hash().empty())
    {
        // Store the message.
        txn_man->info_prepare.push_back(msg->return_node);
        return false;
    }
    else
    {
        if (!checkMsg(msg))
        {
            // If message did not match.
            cout << txn_man->get_hash() << " :: " << msg->hash << "\n";
            cout << get_current_view(get_thd_id()) << " :: " << msg->view << "\n";
            fflush(stdout);
            return false;
        }
    }

    txn_man->add_prep_msg(msg);
    uint64_t prep_cnt = txn_man->decr_prep_rsp_cnt();
    if (prep_cnt == 0)
    {
        txn_man->set_prepared();
        return true;
    }

    return false;
}
