#include "global.h"
#include "message.h"
#include "ycsb.h"
#include "thread.h"
#include "worker_thread.h"
#include "io_thread.h"
#include "math.h"
#include "query.h"
#include "transport.h"
#include "msg_queue.h"
#include "ycsb_query.h"
#include "sim_manager.h"
#include "work_queue.h"
#include "client_query.h"
#include "crypto.h"
#include "timer.h"
#include "chain.h"

void network_test();
void network_test_recv();
void *run_thread(void *);

WorkerThread *worker_thds;
InputThread *input_thds;
OutputThread *output_thds;

// defined in parser.cpp
void parser(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // 0. initialize global data structure
    parser(argc, argv);
#if SEED != 0
    uint64_t seed = SEED + g_node_id;
#else
    uint64_t seed = get_sys_clock();
#endif
    srand(seed);
    printf("Random seed: %ld\n", seed);

    int64_t starttime;
    int64_t endtime;
    starttime = get_server_clock();
    printf("Initializing stats... ");
    fflush(stdout);
    stats.init(g_total_thread_cnt);
    printf("Done\n");

    printf("Initializing DB %s... ", db->dbInstance().c_str());
    fflush(stdout);
    db->Open(string("db-") + to_string(g_node_id));

    printf("DB testing\nInsert key K1 with value V1\n");
    db->Put("K1", "V1");
    printf("Reading value for key K1 = %s\n", db->Get("K1").c_str());

    printf("Done\n");
    fflush(stdout);

    printf("Initializing transport manager... ");
    fflush(stdout);
    tport_man.init();
    printf("Done\n");
    fflush(stdout);

    printf("Initializing simulation... ");
    fflush(stdout);
    simulation = new SimManager;
    simulation->init();
    printf("Done\n");
    fflush(stdout);

    Workload *m_wl = new YCSBWorkload;
    m_wl->init();
    printf("Workload initialized!\n");
    fflush(stdout);

#if NETWORK_TEST
    tport_man.init(g_node_id, m_wl);
    sleep(3);
    if (g_node_id == 0)
        network_test();
    else if (g_node_id == 1)
        network_test_recv();

    return 0;
#endif

    printf("Initializing work queue... ");
    fflush(stdout);
    work_queue.init();
    printf("Done\n");
    printf("Initializing message queue... ");
    fflush(stdout);
    msg_queue.init();
    printf("Done\n");
    printf("Initializing transaction manager pool... ");
    fflush(stdout);
    txn_man_pool.init(m_wl, 0);
    printf("Done\n");
    printf("Initializing transaction pool... ");
    fflush(stdout);
    txn_pool.init(m_wl, 0);
    printf("Done\n");
    printf("Initializing txn node table pool... ");
    fflush(stdout);
    txn_table_pool.init(m_wl, 0);
    printf("Done\n");
    printf("Initializing query pool... ");
    fflush(stdout);
    qry_pool.init(m_wl, 0);
    printf("Done\n");
    printf("Initializing transaction table... ");
    fflush(stdout);
    txn_table.init();
    printf("Done\n");

    printf("Initializing Chain... ");
    fflush(stdout);
    BlockChain = new  BChain();
    printf("Done\n");

#if TIMER_ON
    printf("Initializing timers... ");
    server_timer = new ServerTimer();
#endif

#if LOCAL_FAULT || VIEW_CHANGES
    // Adding a stop_nodes entry for each output thread.
    for (uint i = 0; i < g_send_thread_cnt; i++)
    {
        vector<uint64_t> temp;
        stop_nodes.push_back(temp);
    }
#endif

    for (uint64_t i = 0; i < g_node_cnt + g_client_node_cnt; i++)
    {
        receivedKeys[i] = 0;
    }

#if CRYPTO_METHOD_RSA
    cout << "___________________________________RSAGenerateKeys" << endl;
    auto key = RsaGenerateHexKeyPair(3072);
    g_priv_key = key.privateKey;
    g_pub_keys[g_node_id] = key.publicKey;
    g_public_key = key.publicKey;
    for (uint64_t i = 0; i < g_node_cnt + g_client_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
        receivedKeys[i]++;
    }
    cout << "_____________RSA PUBLIC KEY: " << g_public_key << endl;
    fflush(stdout);

#elif CRYPTO_METHOD_ED25519
    cout << "___________________________________ED25519GenerateKeys" << endl;
    // Initlialize signer and keys for ED25519
    ED25519GenerateKeys(g_priv_key, g_public_key);
    g_pub_keys[g_node_id] = g_public_key;
    for (uint64_t i = 0; i < g_node_cnt + g_client_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
        receivedKeys[i]++;
    }
    //cout << "_____________ED25519 PRIV KEY: " << g_priv_key << endl;
    cout << "_____________ED25519 PUBLIC KEY: " << g_public_key << endl;
    fflush(stdout);

    // TEST:
    byte bKey[CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH];
    copyStringToByte(bKey, g_public_key);
    CryptoPP::ed25519::Verifier verif = CryptoPP::ed25519::Verifier(bKey);

    string message = "hello";
    string signtr;
    StringSource(message, true, new SignerFilter(NullRNG(), signer, new StringSink(signtr)));

    bool valid = true;
    CryptoPP::StringSource(signtr + message, true,
                           new CryptoPP::SignatureVerificationFilter(verif,
                                                                     new CryptoPP::ArraySink((byte *)&valid, sizeof(valid))));
    if (valid == false)
    {
        assert(0);
    }

#endif

#if CRYPTO_METHOD_CMAC_AES
    cout << "___________________________________CMACGenerateKeys" << endl;
    /* 
  When we are using CMAC we just use and exchange a private key between every 
  pairs of nodes. Therefore we need to generate g_node_cnt + g_client_node_cnt -1 keys.
  For the use of this keys we need to keep in mind that id=0 is always for the 
  primary node and the id=n is for the client and that we are generating a useless key 
  for the id g_node_id
  */

    for (unsigned int i = 0; i < g_node_cnt + g_client_node_cnt; i++)
    {
        cmacPrivateKeys[i] = CmacGenerateHexKey(16);
        if (i == g_node_id)
        {
            continue;
        }
        receivedKeys[i]++;

        cout << "_____________CMAC PRIV KEY: " << cmacPrivateKeys[i] << endl;
        fflush(stdout);
    }
    cmacOthersKeys[g_node_id] = cmacPrivateKeys[g_node_id];

#endif

    // 2. spawn multiple threads
    uint64_t thd_cnt = g_thread_cnt;
    uint64_t wthd_cnt = thd_cnt;
    uint64_t rthd_cnt = g_rem_thread_cnt;
    uint64_t sthd_cnt = g_send_thread_cnt;
    uint64_t all_thd_cnt = thd_cnt + rthd_cnt + sthd_cnt;

    assert(all_thd_cnt == g_this_total_thread_cnt);

    pthread_t *p_thds =
        (pthread_t *)malloc(sizeof(pthread_t) * (all_thd_cnt));
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    worker_thds = new WorkerThread[wthd_cnt];
    input_thds = new InputThread[rthd_cnt];
    output_thds = new OutputThread[sthd_cnt];

    endtime = get_server_clock();
    printf("Initialization Time = %ld\n", endtime - starttime);
    fflush(stdout);
    warmup_done = true;
    pthread_barrier_init(&warmup_bar, NULL, all_thd_cnt);

    // spawn and run txns again.
    starttime = get_server_clock();
    simulation->run_starttime = starttime;

    uint64_t id = 0;
    for (uint64_t i = 0; i < wthd_cnt - 1; i++)
    {
        assert(id >= 0 && id < wthd_cnt);
        worker_thds[i].init(id, g_node_id, m_wl);
        pthread_create(&p_thds[id++], &attr, run_thread, (void *)&worker_thds[i]);
        pthread_setname_np(p_thds[id - 1], "s_worker");
    }

    uint64_t ii = id;
    assert(id >= 0 && id < wthd_cnt);
    worker_thds[ii].init(id, g_node_id, m_wl);
    pthread_create(&p_thds[id++], &attr, run_thread, (void *)&worker_thds[ii]);
    pthread_setname_np(p_thds[id - 1], "s_worker");

    for (uint64_t j = 0; j < rthd_cnt; j++)
    {
        assert(id >= wthd_cnt && id < wthd_cnt + rthd_cnt);
        input_thds[j].init(id, g_node_id, m_wl);
        pthread_create(&p_thds[id++], &attr, run_thread, (void *)&input_thds[j]);
        pthread_setname_np(p_thds[id - 1], "s_receiver");
    }

    for (uint64_t j = 0; j < sthd_cnt; j++)
    {
        assert(id >= wthd_cnt + rthd_cnt && id < wthd_cnt + rthd_cnt + sthd_cnt);
        output_thds[j].init(id, g_node_id, m_wl);
        pthread_create(&p_thds[id++], &attr, run_thread, (void *)&output_thds[j]);
        pthread_setname_np(p_thds[id - 1], "s_sender");
    }
#if LOGGING
    log_thds[0].init(id, g_node_id, m_wl);
    pthread_create(&p_thds[id++], &attr, run_thread, (void *)&log_thds[0]);
    pthread_setname_np(p_thds[id - 1], "s_logger");
#endif

    for (uint64_t i = 0; i < all_thd_cnt; i++)
        pthread_join(p_thds[i], NULL);

    endtime = get_server_clock();

    fflush(stdout);
    printf("PASS! SimTime = %f\n", (float)(endtime - starttime) / BILLION);
    if (STATS_ENABLE)
        stats.print(false);

    printf("\n");
    fflush(stdout);
    // Free things
    //tport_man.shutdown();
    //m_wl->index_delete_all();

    // Only for end cleanup.
    //txn_table.free();
    //txn_pool.free_all();
    //txn_table_pool.free_all();
    //qry_pool.free_all();
    //stats.free();
    return 0;
}

void *run_thread(void *id)
{
    Thread *thd = (Thread *)id;
    thd->run();
    return NULL;
}