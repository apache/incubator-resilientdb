#include "global.h"
#include "message.h"
#include "ycsb.h"
#include "thread.h"
#include "io_thread.h"
#include "client_thread.h"
#include "client_query.h"
#include "transport.h"
#include "client_txn.h"
#include "msg_queue.h"
#include "work_queue.h"
#include "crypto.h"
#include "timer.h"
#include "smart_contract_txn.h"
#include <SimpleAmqpClient/SimpleAmqpClient.h>

using namespace AmqpClient;

void *f(void *);
void *g(void *);
void *worker(void *);
void *nn_worker(void *);
void *send_worker(void *);
void network_test();
void network_test_recv();
void *run_thread(void *);

ClientThread *client_thds;
InputThread *input_thds;
OutputThread *output_thds;

// defined in parser.cpp
void parser(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    printf("Running client...\n\n");
    // 0. initialize global data structure
    parser(argc, argv);
    assert(g_node_id >= g_node_cnt);
    uint64_t seed = get_sys_clock();
    srand(seed);
    printf("Random seed: %ld\n", seed);

    int64_t starttime;
    int64_t endtime;
    starttime = get_server_clock();
    // per-partition malloc
    printf("Initializing stats... ");
    fflush(stdout);
    stats.init(g_total_client_thread_cnt);
    printf("Done\n");
    printf("Initializing transport manager... ");
    fflush(stdout);
    tport_man.init();
    printf("Done\n");
    printf("Initializing client manager... ");
#if BANKING_SMART_CONTRACT
    Workload *m_wl = new SCWorkload;
#else
    Workload *m_wl = new YCSBWorkload;
#endif
    m_wl->Workload::init();
    printf("workload initialized!\n");

    printf("Initializing simulation... ");
    fflush(stdout);
    simulation = new SimManager;
    simulation->init();
    printf("Done\n");
#if NETWORK_TEST
    tport_man.init(g_node_id, m_wl);
    sleep(3);
    if (g_node_id == 0)
        network_test();
    else if (g_node_id == 1)
        network_test_recv();

    return 0;
#endif

    fflush(stdout);
    client_man.init();
    printf("Done\n");
    printf("Initializing work queue... ");
    fflush(stdout);
    work_queue.init();
    printf("Done\n");
    fflush(stdout);
#if TIMER_ON
    printf("Initializing timers... ");
    fflush(stdout);
    client_timer = new ClientTimer();
#endif

    printf("Reading in Keys");

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
    //cout << "_____________ED25519 PRIV KEY IS: " << g_priv_key << endl;
    cout << "_____________ED25519 PUBLIC KEY IS: " << g_public_key << endl;
    fflush(stdout);
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
        //cmacPrivateKeys[i] = CmacGenerateHexKey(16);
        //std::cout << "Receiving CMAC object" << std::endl;
        //CMACsend[i] = CMACgenerateInstance(cmacPrivateKeys[i]);
        receivedKeys[i]++;

        cout << "_____________CMAC PRIV KEY: " << cmacPrivateKeys[i] << endl;
        fflush(stdout);
    }
    cmacOthersKeys[g_node_id] = cmacPrivateKeys[g_node_id];

#endif

    printf("Done\n");

    // 2. spawn multiple threads
    uint64_t thd_cnt = g_client_thread_cnt;
    uint64_t cthd_cnt = thd_cnt;
    uint64_t rthd_cnt = g_client_rem_thread_cnt;
    uint64_t sthd_cnt = g_client_send_thread_cnt;
    uint64_t all_thd_cnt = thd_cnt + rthd_cnt + sthd_cnt;
    assert(all_thd_cnt == g_this_total_thread_cnt);

    pthread_t *p_thds =
        (pthread_t *)malloc(sizeof(pthread_t) * (all_thd_cnt));
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    client_thds = new ClientThread[cthd_cnt];
    input_thds = new InputThread[rthd_cnt];
    output_thds = new OutputThread[sthd_cnt];
    //// query_queue should be the last one to be initialized!!!
    // because it collects txn latency
    printf("Initializing message queue... ");
    msg_queue.init();
    printf("Done\n");
    printf("Initializing client query queue... ");
    fflush(stdout);
    //client_query_queue.init(m_wl);
    client_query_queue.init();
    printf("Done\n");
    fflush(stdout);

    endtime = get_server_clock();
    printf("Initialization Time = %ld\n", endtime - starttime);
    fflush(stdout);
    warmup_done = true;
    pthread_barrier_init(&warmup_bar, NULL, all_thd_cnt);

    uint64_t cpu_cnt = 0;
    cpu_set_t cpus;
    // spawn and run txns again.
    starttime = get_server_clock();
    simulation->run_starttime = starttime;

    uint64_t id = 0;
    CPU_ZERO(&cpus);
    CPU_SET(cpu_cnt, &cpus);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpus);
    cpu_cnt++;
    for (uint64_t i = 0; i < thd_cnt; i++)
    {
        CPU_ZERO(&cpus);
        //#if TPORT_TYPE_IPC
        //        CPU_SET(g_node_id * thd_cnt + cpu_cnt, &cpus);
        //#elif !SET_AFFINITY
        //        CPU_SET(g_node_id * thd_cnt + cpu_cnt, &cpus);
        //#else
        CPU_SET(cpu_cnt, &cpus);
        //#endif
        //    cpu_cnt = (cpu_cnt + 1) % g_servers_per_client;
        //        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        client_thds[i].init(id, g_node_id, m_wl);
        pthread_create(&p_thds[id++], &attr, run_thread, (void *)&client_thds[i]);
        pthread_setname_np(p_thds[id - 1], "worker");
        cpu_cnt++;
    }

    for (uint64_t j = 0; j < rthd_cnt; j++)
    {
        input_thds[j].init(id, g_node_id, m_wl);
        CPU_ZERO(&cpus);
        CPU_SET(cpu_cnt, &cpus);
        //        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&p_thds[id++], &attr, run_thread, (void *)&input_thds[j]);
        pthread_setname_np(p_thds[id - 1], "receiver");
        cpu_cnt++;
    }

    for (uint64_t i = 0; i < sthd_cnt; i++)
    {
        output_thds[i].init(id, g_node_id, m_wl);
        CPU_ZERO(&cpus);
        CPU_SET(cpu_cnt, &cpus);
        //        pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
        pthread_create(&p_thds[id++], &attr, run_thread, (void *)&output_thds[i]);
        pthread_setname_np(p_thds[id - 1], "sender");
        cpu_cnt++;
    }
    for (uint64_t i = 0; i < all_thd_cnt; i++)
        pthread_join(p_thds[i], NULL);

    endtime = get_server_clock();

    fflush(stdout);
    printf("CLIENT PASS! SimTime = %ld\n", endtime - starttime);
    if (STATS_ENABLE)
        stats.print_client(false);
    fflush(stdout);

    stats.free();
    return 0;
}

void *run_thread(void *id)
{
    Thread *thd = (Thread *)id;
    thd->run();
    return NULL;
}

void network_test()
{
    /*

	ts_t start;
	ts_t end;
	double time;
	int bytes;
	for(int i=4; i < 257; i+=4) {
		time = 0;
		for(int j=0;j < 1000; j++) {
			start = get_sys_clock();
			tport_man.simple_send_msg(i);
			while((bytes = tport_man.simple_recv_msg()) == 0) {}
			end = get_sys_clock();
			assert(bytes == i);
			time += end-start;
		}
		time = time/1000;
		printf("Network Bytes: %d, s: %f\n",i,time/BILLION);
        fflush(stdout);
	}
  */
}

void consumeMessage() {
  Channel::OpenOpts openOpts = Channel::OpenOpts();
  openOpts.host = "localhost";
  openOpts.port = 5673;
  Channel::OpenOpts::BasicAuth basicAuth = Channel::OpenOpts::BasicAuth("guest","guest");
  openOpts.auth = basicAuth;
  Channel::ptr_t connection = Channel::Open(openOpts);
  std::string consumer_tag = connection->BasicConsume("custom_queue", "");
  Envelope::ptr_t envelope = connection->BasicConsumeMessage(consumer_tag);
  cout << envelope->Message()->Body();
  connection->BasicConsumeMessage(consumer_tag, envelope, 10);  // 10 ms timeout
}

void publishMessage() {
  Channel::OpenOpts openOpts = Channel::OpenOpts();
  openOpts.host = "localhost";
  openOpts.port = 5673;
  Channel::OpenOpts::BasicAuth basicAuth = Channel::OpenOpts::BasicAuth("guest","guest");
  openOpts.auth = basicAuth;
  Channel::ptr_t connection = Channel::Open(openOpts);
  string producerMessage = "this is a test message 2";
  connection->BasicPublish("TestExchange", "TQ1",
                           BasicMessage::Create(producerMessage));
}

void network_test_recv()
{
    /*
	int bytes;
	while(1) {
		if( (bytes = tport_man.simple_recv_msg()) > 0)
			tport_man.simple_send_msg(bytes);
	}
  */
}
