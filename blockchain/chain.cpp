#include "chain.h"

/* Set the identifier for the block. */
void BChainStruct::set_txn_id(uint64_t tid)
{
	txn_id = tid;
}	

/* Get the identifier of this block. */
uint64_t BChainStruct::get_txn_id()
{
	return txn_id;
}

/* Store the BatchRequests to this block. */
void BChainStruct::add_batch(BatchRequests *msg) {
	char *buf = create_msg_buffer(msg);
	Message *deepMsg = deep_copy_msg(buf, msg);
	batch_info = (BatchRequests *)deepMsg;
	delete_msg_buffer(buf);
}	

/* Store the commit messages to this block. */
void BChainStruct::add_commit_proof(Message *msg) {
	char *buf = create_msg_buffer(msg);
	Message *deepMsg = deep_copy_msg(buf, msg);
	commit_proof.push_back(deepMsg);
	delete_msg_buffer(buf);
}

/* Release the contents of the block. */
void BChainStruct::release_data() {
	Message::release_message(this->batch_info);

	PBFTCommitMessage *cmsg;
	while(this->commit_proof.size()>0)
	{
		cmsg = (PBFTCommitMessage *)this->commit_proof[0];
		this->commit_proof.erase(this->commit_proof.begin());
		Message::release_message(cmsg);
	}	
}


/****************************************/

/* Add a block to the chain. */
void BChain::add_block(TxnManager *txn) {
	BChainStruct *blk = (BChainStruct *)mem_allocator.alloc(sizeof(BChainStruct));
	new (blk) BChainStruct();

	blk->set_txn_id(txn->get_txn_id());
	blk->add_batch(txn->batchreq);

	for(uint64_t i=0; i<txn->commit_msgs.size(); i++) {
		blk->add_commit_proof(txn->commit_msgs[i]);
	}	
	
	chainLock.lock();
	   bchain_map.push_back(blk);
	chainLock.unlock();
}

/* Remove a block from the chain bbased on its identifier. */
void BChain::remove_block(uint64_t tid)
{
	BChainStruct *blk;
	bool found = false;

	chainLock.lock();
	   for (uint64_t i = 0; i < bchain_map.size(); i++)
	   {
	   	blk = bchain_map[i];
	   	if (blk->get_txn_id() == tid)
	   	{
	   		bchain_map.erase(bchain_map.begin() + i);
	   		found = true;
	   		break;
	   	}
	   }
	chainLock.unlock();

	if(found) {
		blk->release_data();
		mem_allocator.free(blk, sizeof(BChainStruct));
	}	
}

/*****************************************/

BChain *BlockChain;
std::mutex chainLock;
