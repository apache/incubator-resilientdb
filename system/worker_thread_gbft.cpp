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

#if CONSENSUS == PBFT && GBFT
/**
 * Processes an incoming client batch and sends a Pre-prepare message to al replicas.
 *
 * This function assumes that a client sends a batch of transactions and 
 * for each transaction in the batch, a separate transaction manager is created. 
 * Next, this batch is forwarded to all the replicas as a BatchRequests Message, 
 * which corresponds to the Pre-Prepare stage in the PBFT protocol.
 *
 * @param msg Batch of Transactions of type CientQueryBatch from the client.
 * @return RC
 */
RC WorkerThread::process_client_batch(Message *msg)
{
	ClientQueryBatch *clbtch = (ClientQueryBatch *)msg;

	// printf("ClientQueryBatch: %ld, THD: %ld :: CL: %ld :: RQ: %ld\n", msg->txn_id, get_thd_id(), msg->return_node_id, clbtch->cqrySet[0]->requests[0]->key);
	// fflush(stdout);

	// Authenticate the client signature.
	validate_msg(clbtch);

#if VIEW_CHANGES
	// If message forwarded to the non-primary.
	if (g_node_id != get_current_view(get_thd_id()))
	{
		client_query_check(clbtch);
		cout << "returning...   " << get_current_view(get_thd_id()) << endl;
		return RCOK;
	}

	// Partial failure of Primary 0.
	fail_primary(msg, 10 * BILLION);
#endif

	// Initialize all transaction mangers and Send BatchRequests message.
	create_and_send_batchreq(clbtch, clbtch->txn_id);

	return RCOK;
}

/**
 * Process incoming BatchRequests message from the Primary.
 *
 * This function is used by the non-primary or backup replicas to process an incoming
 * BatchRequests message sent by the primary replica. This processing would require 
 * sending messages of type PBFTPrepMessage, which correspond to the Prepare phase of 
 * the PBFT protocol. Due to network delays, it is possible that a repica may have 
 * received some messages of type PBFTPrepMessage and PBFTCommitMessage, prior to 
 * receiving this BatchRequests message.
 *
 * @param msg Batch of Transactions of type BatchRequests from the primary.
 * @return RC
 */
RC WorkerThread::process_batch(Message *msg)
{
	uint64_t cntime = get_sys_clock();

	BatchRequests *breq = (BatchRequests *)msg;

	// printf("BatchRequests: TID:%ld : VIEW: %ld : THD: %ld\n",breq->txn_id, breq->view, get_thd_id());
	// fflush(stdout);

	// Assert that only a non-primary replica has received this message.
	assert(g_node_id != get_current_view(get_thd_id()));

	// Check if the message is valid.
	validate_msg(breq);

#if VIEW_CHANGES
	// Store the batch as it could be needed during view changes.
	store_batch_msg(breq);
#endif

	// Allocate transaction managers for all the transactions in the batch.
	set_txn_man_fields(breq, 0);

#if TIMER_ON
	// The timer for this client batch stores the hash of last request.
	add_timer(breq, txn_man->get_hash());
#endif

	// Storing the BatchRequests message.
	txn_man->set_primarybatch(breq);

	// Send Prepare messages.
	txn_man->send_pbft_prep_msgs();

	// End the counter for pre-prepare phase as prepare phase starts next.
	double timepre = get_sys_clock() - cntime;
	INC_STATS(get_thd_id(), time_pre_prepare, timepre);

	// Only when BatchRequests message comes after some Prepare message.
	for (uint64_t i = 0; i < txn_man->info_prepare.size(); i++)
	{
		// Decrement.
		uint64_t num_prep = txn_man->decr_prep_rsp_cnt();
		if (num_prep == 0)
		{
			txn_man->set_prepared();
			break;
		}
	}

	// If enough Prepare messages have already arrived.
	if (txn_man->is_prepared())
	{
		// Send Commit messages.
		txn_man->send_pbft_commit_msgs();

		double timeprep = get_sys_clock() - txn_man->txn_stats.time_start_prepare - timepre;
		INC_STATS(get_thd_id(), time_prepare, timeprep);
		double timediff = get_sys_clock() - cntime;

		// Check if any Commit messages arrived before this BatchRequests message.
		for (uint64_t i = 0; i < txn_man->info_commit.size(); i++)
		{
			uint64_t num_comm = txn_man->decr_commit_rsp_cnt();
			if (num_comm == 0)
			{
				txn_man->set_committed();
				break;
			}
		}

		// If enough Commit messages have already arrived.
		if (txn_man->is_committed())
		{
#if TIMER_ON
			// End the timer for this client batch.
			remove_timer(txn_man->hash);
#endif
			// Proceed to executing this batch of transactions.
			send_execute_msg();

			// End the commit counter.
			INC_STATS(get_thd_id(), time_commit, get_sys_clock() - txn_man->txn_stats.time_start_commit - timediff);
		}
	}
	else
	{
		// Although batch has not prepared, still some commit messages could have arrived.
		for (uint64_t i = 0; i < txn_man->info_commit.size(); i++)
		{
			txn_man->decr_commit_rsp_cnt();
		}
	}

	// Release this txn_man for other threads to use.
	bool ready = txn_man->set_ready();
	assert(ready);

	// UnSetting the ready for the txn id representing this batch.
	txn_man = get_transaction_manager(msg->txn_id, 0);
	unset_ready_txn(txn_man);

	return RCOK;
}

/**
 * Processes incoming Prepare message.
 *
 * This functions precessing incoming messages of type PBFTPrepMessage. If a replica 
 * received 2f identical Prepare messages from distinct replicas, then it creates 
 * and sends a PBFTCommitMessage to all the other replicas.
 *
 * @param msg Prepare message of type PBFTPrepMessage from a replica.
 * @return RC
 */
RC WorkerThread::process_pbft_prep_msg(Message *msg)
{
	// cout << "PBFTPrepMessage: TID: " << msg->txn_id << " FROM: " << msg->return_node_id << endl;
	// fflush(stdout);

	// Start the counter for prepare phase.
	if (txn_man->prep_rsp_cnt == 2 * g_min_invalid_nodes)
	{
		txn_man->txn_stats.time_start_prepare = get_sys_clock();
	}

	// Check if the incoming message is valid.
	PBFTPrepMessage *pmsg = (PBFTPrepMessage *)msg;
	validate_msg(pmsg);

	// Check if sufficient number of Prepare messages have arrived.
	if (prepared(pmsg))
	{
		// Send Commit messages.
		txn_man->send_pbft_commit_msgs();

		// End the prepare counter.
		INC_STATS(get_thd_id(), time_prepare, get_sys_clock() - txn_man->txn_stats.time_start_prepare);
	}

	return RCOK;
}

/**
 * Checks if the incoming PBFTCommitMessage can be accepted.
 *
 * This functions checks if the hash and view of the commit message matches that of 
 * the Pre-Prepare message. Once 2f+1 messages are received it returns a true and 
 * sets the `is_committed` flag for furtue identification.
 *
 * @param msg PBFTCommitMessage.
 * @return bool True if the transactions of this batch can be executed.
 */
bool WorkerThread::committed_local(PBFTCommitMessage *msg)
{
	// cout << "Check Commit: TID: " << txn_man->get_txn_id() << "\n";
	// fflush(stdout);

	// Once committed is set for this transaction, no further processing.
	if (txn_man->is_committed())
	{
		return false;
	}

	// If BatchRequests messages has not arrived, then hash is empty; return false.
	if (txn_man->get_hash().empty())
	{
		//cout << "hash empty: " << txn_man->get_txn_id() << "\n";
		//fflush(stdout);
		txn_man->info_commit.push_back(msg->return_node);
		return false;
	}
	else
	{
		if (!checkMsg(msg))
		{
			// If message did not match.
			//cout << txn_man->get_hash() << " :: " << msg->hash << "\n";
			//cout << get_current_view(get_thd_id()) << " :: " << msg->view << "\n";
			//fflush(stdout);
			return false;
		}
	}

	uint64_t comm_cnt = txn_man->decr_commit_rsp_cnt();
	if (comm_cnt == 0 && txn_man->is_prepared())
	{
		txn_man->set_committed();
		return true;
	}

	return false;
}

/**
 * Processes incoming Commit message.
 *
 * This functions precessing incoming messages of type PBFTCommitMessage. If a replica 
 * received 2f+1 identical Commit messages from distinct replicas, then it asks the 
 * execute-thread to execute all the transactions in this batch.
 *
 * @param msg Commit message of type PBFTCommitMessage from a replica.
 * @return RC
 */
RC WorkerThread::process_pbft_commit_msg(Message *msg)
{
	// cout << "PBFTCommitMessage: TID " << msg->txn_id / get_batch_size() << " FROM: " << msg->return_node_id << "\n";
	// fflush(stdout);

	if (txn_man->commit_rsp_cnt == 2 * g_min_invalid_nodes + 1)
	{
		txn_man->txn_stats.time_start_commit = get_sys_clock();
	}

	// Check if message is valid.
	PBFTCommitMessage *pcmsg = (PBFTCommitMessage *)msg;
	validate_msg(pcmsg);
	txn_man->add_commit_msg(pcmsg);

	// Check if sufficient number of Commit messages have arrived.
	if (committed_local(pcmsg))
	{
#if TIMER_ON
		// End the timer for this client batch.
		remove_timer(txn_man->hash);
#endif

#if GBFT
		if (is_primary_node(get_thd_id(), g_node_id) && g_node_cnt > gbft_cluster_size)
		{
			GeoBFTCommitCertificateMessage *ccm = (GeoBFTCommitCertificateMessage *)Message::create_message(txn_man, GBFT_COMMIT_CERTIFICATE_MSG);
			ccm->txn_id = txn_man->get_txn_id();

			vector<uint64_t> dest;
			for (uint64_t i = 0; i < g_node_cnt; i++)
			{
				if (is_in_same_cluster(g_node_id, i) || i % gbft_cluster_size > g_min_invalid_nodes)
					continue;
				dest.push_back(i);
			}

			msg_queue.enqueue(get_thd_id(), ccm, dest);
			dest.clear();
		}
#endif

		// Add this message to execute thread's queue.
		send_execute_msg();

		INC_STATS(get_thd_id(), time_commit, get_sys_clock() - txn_man->txn_stats.time_start_commit);
	}

	return RCOK;
}

#if GBFT
RC WorkerThread::process_gbft_commit_certificate_msg(Message *msg)
{

	GeoBFTCommitCertificateMessage *ccm = (GeoBFTCommitCertificateMessage *)msg;
	if (curr_next_index() > ccm->index[0])
	{
		return RCOK;
	}

	// TxnManager *tman = txn_table.get_transaction_manager(get_thd_id(), ccm->index[g_batch_size - 1], 0);
	if (gbft_ccm_checklist.exists(ccm->hash))
	{
		return RCOK;
	}

	// printf("PROCESS CCM   %ld\tFROM: %ld THREAD: %ld\n", msg->txn_id / 100, msg->return_node_id, get_thd_id());
	// fflush(stdout);


	// ONLY NON PRIMARY NODES.
	assert(ccm->validate());

	if (!is_in_same_cluster(msg->return_node_id, g_node_id))
	{

		char *buf = create_msg_buffer(ccm);
		Message *deepCopyMsg = deep_copy_msg(buf, ccm);
		((GeoBFTCommitCertificateMessage *)deepCopyMsg)->forwarding_from = msg->return_node_id;
		delete_msg_buffer(buf);

		vector<uint64_t> dest;
		for (uint64_t i = 0; i < g_node_cnt; i++)
		{
			if (i == g_node_id || !is_in_same_cluster(g_node_id, i))
				continue;
			dest.push_back(i);
		}
		msg_queue.enqueue(get_thd_id(), deepCopyMsg, dest);
		dest.clear();
	}

	// Allocate transaction manager for all the requests in batch.
	uint64_t i;
	for (i = 0; i < get_batch_size() - 1; i++)
	{
		uint64_t txn_id = ccm->index[i];

		txn_man = get_transaction_manager(txn_id, 0);
		txn_man->register_thread(this);
		txn_man->return_id = msg->return_node_id;

		init_txn_man(ccm->requestMsg[i]);
	}

	uint64_t txn_id = ccm->index[i];
	txn_man = get_transaction_manager(txn_id, 0);
	txn_man->return_id = msg->return_node_id;
	init_txn_man(ccm->requestMsg[i]);
	gbft_ccm_checklist.add(ccm->hash);

	Message *emsg = Message::create_message(txn_man, EXECUTE_MSG);
	emsg->return_node_id = msg->return_node_id;
	work_queue.enqueue(get_thd_id(), emsg, false);

	return RCOK;
}
#endif // GBFT

#endif // Main GBFT
