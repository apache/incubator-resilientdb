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

#if MIN_PBFT_ALL_TO_ALL

uint64_t PrepCertificateMessage::get_size()
{
    uint64_t size = Message::mget_size();
    size += sizeof(view);
    size += sizeof(uint64_t) * index.size();
    size += sizeof(hashSize);
    size += hash.length();
    size += sizeof(uint64_t) * signOwner.size();
    size += sizeof(uint64_t) * signSize.size();
    for (uint i = 0; i < g_min_invalid_nodes; i++)
    {
        size += signatures[i].length();
    }
    return size;
}

void PrepCertificateMessage::copy_from_txn(TxnManager *txn)
{

    // assert(is_primary_node(g_node_id)); // only primary creates this message

    // Setting txn_id 2 less than the actual value.

    this->txn_id = txn->get_txn_id() - 2;

    // Initialization
    this->index.init(get_batch_size());
    this->signatures.resize(g_min_invalid_nodes);
    this->signOwner.init(g_min_invalid_nodes);
    this->signSize.init(g_min_invalid_nodes);
    this->hash = txn->hash;
    this->hashSize = txn->hashSize;

    for (uint64_t txnid = txn->get_txn_id() - g_batch_size + 1; txnid < txn->get_txn_id() + 1; txnid++)
    {

        this->index.add(txnid);
    }
    PBFTPrepMessage *pmsg;
    uint64_t i = 0;
    for (i = 0; i < txn->prep_msgs.size(); i++)
    {
        if (i >= (uint64_t)(g_min_invalid_nodes))
            break;
        pmsg = txn->prep_msgs[i];
        // One time to fill ccm itself
        this->signOwner.add(pmsg->return_node_id);
        this->signatures[i] = pmsg->signature;
        this->signSize.add(pmsg->signature.length());
    }
    this->view = txn->prep_msgs[i]->view;
}

void PrepCertificateMessage::release()
{
    index.release();
    signOwner.release();
    signSize.release();
    vector<string>().swap(signatures);
}

void PrepCertificateMessage::copy_to_txn(TxnManager *txn)
{
    Message::mcopy_to_txn(txn);
}

void PrepCertificateMessage::copy_from_buf(char *buf)
{
    Message::mcopy_from_buf(buf);

    uint64_t ptr = Message::mget_size();

    uint64_t elem;
    // Initialization
    index.init(g_batch_size);
    signatures.resize(g_min_invalid_nodes);
    signSize.init(g_min_invalid_nodes);
    signOwner.init(g_min_invalid_nodes);

    COPY_VAL(elem, buf, ptr);
    this->view = elem;

    COPY_VAL(elem, buf, ptr);
    this->hashSize = elem;

    string temp_hash;
    ptr = buf_to_string(buf, ptr, temp_hash, this->hashSize);
    this->hash = temp_hash;

    for (uint i = 0; i < g_batch_size; i++)
    {
        COPY_VAL(elem, buf, ptr);
        index.add(elem);
    }

    for (uint i = 0; i < g_min_invalid_nodes; i++)
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

void PrepCertificateMessage::copy_to_buf(char *buf)
{
    Message::mcopy_to_buf(buf);
    uint64_t ptr = Message::mget_size();

    COPY_BUF(buf, view, ptr);
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
    }
    for (uint i = 0; i < g_min_invalid_nodes; i++)
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

void PrepCertificateMessage::sign(uint64_t dest_node_id)
{
#if USE_CRYPTO
    string message = toString();
    signingClientNode(message, this->signature, this->pubKey, dest_node_id);
#else
    this->signature = "0";
#endif
    this->sigSize = this->signature.size();
    this->keySize = this->pubKey.size();
}

string PrepCertificateMessage::toString()
{
    string message = "";
    message += this->hash;
    message += this->view;
    message += this->txn_id;

    return message;
}

// makes sure message is valid, returns true for false
bool PrepCertificateMessage::validate()
{
#if USE_CRYPTO
    string message = this->toString();
    uint64_t return_node = this->return_node_id;
    if (!validateClientNode(message, g_pub_keys[return_node], this->signature, return_node))
    {
        assert(0);
    }

#endif
    return true;
}

#endif
