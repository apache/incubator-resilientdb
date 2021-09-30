#include "mem_alloc.h"
#include "query.h"
#include "ycsb_query.h"
#include "ycsb.h"
#include "global.h"
#include "message.h"
#include "crypto.h"
#include <fstream>
#include <ctime>
#include <string>

#if GBFT

uint64_t GeoBFTCommitCertificateMessage::get_size()
{
    uint64_t size = Message::mget_size();
    size += sizeof(view);
    size += sizeof(forwarding_from);
    size += sizeof(uint64_t) * index.size();
    size += sizeof(hashSize);
    size += hash.length();
    size += sizeof(uint64_t) * signOwner.size();
    size += sizeof(uint64_t) * signSize.size();
    for (uint i = 0; i < g_batch_size; i++)
    {
        size += requestMsg[i]->get_size();
    }
    for (uint i = 0; i < (g_min_invalid_nodes * 2); i++)
    {
        size += signatures[i].length();
    }
    return size;
}

void GeoBFTCommitCertificateMessage::copy_from_txn(TxnManager *txn)
{
    
    // assert(is_primary_node(g_node_id)); // only primary creates this message

    // Setting txn_id 2 less than the actual value.

    this->txn_id = txn->get_txn_id() - 2;

    // Initialization
    this->index.init(get_batch_size());
    this->requestMsg.resize(get_batch_size());
    this->signatures.resize(g_min_invalid_nodes * 2);
    this->signOwner.init(g_min_invalid_nodes * 2);
    this->signSize.init(g_min_invalid_nodes * 2);
    this->hash = txn->hash;
    this->hashSize = txn->hashSize;
    

    for (uint64_t txnid = txn->get_txn_id() - g_batch_size + 1; txnid < txn->get_txn_id() + 1; txnid++)
    {
        //sajjad
        TxnManager *t_man = txn_table.get_transaction_manager(0, txnid, 0);
        uint64_t idx = txnid % get_batch_size();

        YCSBQuery *yquery = ((YCSBQuery *)t_man->query);

         YCSBClientQueryMessage *yqry = (YCSBClientQueryMessage *)Message::create_message(CL_QRY);
        ((ClientQueryMessage *)yqry)->client_startts = t_man->client_startts;
        yqry->return_node = t_man->client_id;
        yqry->requests.init(g_req_per_query);
        for (uint64_t i = 0; i < g_req_per_query; i ++) {			
            ycsb_request * req = (ycsb_request*) mem_allocator.alloc(sizeof(ycsb_request));
            req->key = yquery->requests[i]->key;
            req->value = yquery->requests[i]->value;
            yqry->requests.add(req);
        }
        this->requestMsg[idx] = yqry;

        this->index.add(txnid);
        
    }
    PBFTCommitMessage *cmsg;
    uint64_t i = 0;
    for (i = 0; i < txn->commit_msgs.size(); i++)
    {
        if (i >= (uint64_t)(g_min_invalid_nodes * 2))
            break;
        cmsg = txn->commit_msgs[i];
        // One time to fill ccm itself
        this->signOwner.add(cmsg->return_node_id);
        this->signatures[i] = cmsg->signature;
        this->signSize.add(cmsg->signature.length());
    }
    this->view = txn->commit_msgs[i]->view;
}

void GeoBFTCommitCertificateMessage::release()
{
    index.release();
    signOwner.release();
    signSize.release();
    for (size_t i = 0; i < requestMsg.size(); i++)
    {
        requestMsg[i]->release();
        mem_allocator.free(requestMsg[i], sizeof(YCSBClientQueryMessage));
    }
    vector<YCSBClientQueryMessage *>().swap(requestMsg);
    vector<string>().swap(signatures);
}

void GeoBFTCommitCertificateMessage::copy_to_txn(TxnManager *txn)
{
    Message::mcopy_to_txn(txn);
}

void GeoBFTCommitCertificateMessage::copy_from_buf(char *buf)
{
    Message::mcopy_from_buf(buf);

    uint64_t ptr = Message::mget_size();

    uint64_t elem;
    // Initialization
    index.init(g_batch_size);
    requestMsg.resize(g_batch_size);
    signatures.resize(g_min_invalid_nodes * 2);
    signSize.init(g_min_invalid_nodes * 2);
    signOwner.init(g_min_invalid_nodes * 2);

    COPY_VAL(elem, buf, ptr);
    this->view = elem;

    COPY_VAL(elem, buf, ptr);
    this->forwarding_from = elem;

    COPY_VAL(elem, buf, ptr);
    this->hashSize = elem;

    string temp_hash;
    ptr = buf_to_string(buf, ptr, temp_hash, this->hashSize);
    this->hash = temp_hash;

    for (uint i = 0; i < g_batch_size; i++)
    {
        COPY_VAL(elem, buf, ptr);
        index.add(elem);

        Message *msg = create_message(&buf[ptr]);
        ptr += msg->get_size();
        requestMsg[i] = ((YCSBClientQueryMessage *)msg);
    }

    for (uint i = 0; i < (g_min_invalid_nodes * 2); i++)
    {
        COPY_VAL(elem, buf, ptr);
        signOwner.add(elem);

        COPY_VAL(elem, buf, ptr);
        signSize.add(elem);

        string tsign;
        ptr = buf_to_string(buf, ptr, tsign, signSize[i]);
        signatures[i] = tsign;
    }

    assert(ptr == get_size());
}

void GeoBFTCommitCertificateMessage::copy_to_buf(char *buf)
{
    Message::mcopy_to_buf(buf);
    uint64_t ptr = Message::mget_size();
    
    COPY_BUF(buf,view,ptr);
    COPY_BUF(buf,forwarding_from,ptr);
    COPY_BUF(buf, this->hashSize, ptr);

    string hstr = this->hash;
    char v;
    for (uint j = 0; j < hstr.size(); j++)
    {
        v = hstr[j];
        COPY_BUF(buf, v, ptr);
    }
    
    uint64_t elem;
    for (uint i = 0; i < g_batch_size; i++)
    {
        elem = index[i];
        COPY_BUF(buf, elem, ptr);

        //copy client request stored in message to buf
        requestMsg[i]->copy_to_buf(&buf[ptr]);
        ptr += requestMsg[i]->get_size();
    }
    for (uint i = 0; i < (g_min_invalid_nodes * 2); i++)
    {

        elem = signOwner[i];
        COPY_BUF(buf, elem, ptr);
        elem = signSize[i];
        COPY_BUF(buf, elem, ptr);
        char v;
        string sstr = signatures[i];
        for (uint j = 0; j < sstr.size(); j++)
        {
            v = sstr[j];
            COPY_BUF(buf, v, ptr);
        }
    }

    assert(ptr == get_size());
}

void GeoBFTCommitCertificateMessage::sign(uint64_t dest_node_id){
#if USE_CRYPTO
    string message = toString();
    signingClientNode(message, this->signature, this->pubKey, dest_node_id);
#else
    this->signature = "0";
#endif
    this->sigSize = this->signature.size();
    this->keySize = this->pubKey.size();
}

string GeoBFTCommitCertificateMessage::toString()
{
    string message = "";
    message += this->hash;
    message += this->view;
    message += this->txn_id;

    return message;
}

//makes sure message is valid, returns true for false
bool GeoBFTCommitCertificateMessage::validate()
{
#if USE_CRYPTO
    string message = this->toString();
    uint64_t return_node =  this->forwarding_from != (uint64_t)-1 ? this->forwarding_from : this->return_node_id;
    if (!validateClientNode(message, g_pub_keys[return_node], this->signature, return_node))
    {
        assert(0);
    }
    for (uint64_t i = 0; i < this->signatures.size(); i++)
    {


        string signString = std::to_string(this->view);
        signString += '_' + std::to_string(this->txn_id - get_batch_size() + 1); // index
        signString += '_' + this->hash;
        signString += '_' + std::to_string(this->signOwner[i]);
        signString += '_' + to_string(this->txn_id); // end_index
        assert(validateClientNode(signString, g_pub_keys[this->signOwner[i]], this->signatures[i], this->signOwner[i]));
    }

#endif
    string batchStr = "";
    for(uint64_t i=0; i<get_batch_size(); i++) {
        batchStr += this->requestMsg[i]->getString();
	}
    if (this->hash != calculateHash(batchStr)){
        assert(0);
    }
    return true;
}

#endif
