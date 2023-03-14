#include <nng/nng.h>
#include "global.h"
#include "transport.h"
#include "nn.hpp"
#include "query.h"
#include "message.h"

#define MAX_IFADDR_LEN 20 // max # of characters in name of address

void Transport::read_ifconfig(const char *ifaddr_file)
{

    ifaddr = new char *[g_total_node_cnt];

    uint64_t cnt = 0;
    printf("Reading ifconfig file: %s\n", ifaddr_file);
    ifstream fin(ifaddr_file);
    string line;
    while (getline(fin, line))
    {
        // memcpy(ifaddr[cnt],&line[0],12);
        ifaddr[cnt] = new char[line.length() + 1];
        strcpy(ifaddr[cnt], &line[0]);
        printf("%ld: %s\n", cnt, ifaddr[cnt]);
        cnt++;
    }
    assert(cnt == g_total_node_cnt);
}

uint64_t Transport::get_socket_count()
{
    uint64_t sock_cnt = 0;
    if (ISCLIENT)
        sock_cnt = (g_total_node_cnt)*2 + g_client_send_thread_cnt * g_servers_per_client;
    else
        sock_cnt = (g_total_node_cnt)*2 + g_client_send_thread_cnt;
    return sock_cnt;
}

string Transport::get_path()
{
    string path;
#if SHMEM_ENV
    path = "/dev/shm/";
#else
    char *cpath;
    cpath = getenv("SCHEMA_PATH");
    if (cpath == NULL)
        path = "./";
    else
        path = string(cpath);
#endif
    path += "ifconfig.txt";
    return path;
}

Socket *Transport::get_socket()
{
    // Socket * socket = new Socket;
    Socket *socket = (Socket *)mem_allocator.align_alloc(sizeof(Socket));
    new (socket) Socket();
    int timeo = 1000;  // timeout in ms
    int stimeo = 1000; // timeout in ms
    socket->sock.setsockopt_ms(NNG_OPT_RECVTIMEO, timeo);
    socket->sock.setsockopt_ms(NNG_OPT_SENDTIMEO, stimeo);
    return socket;
}

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id)
{
    uint64_t port_id = TPORT_PORT;
    port_id += g_total_node_cnt * dest_node_id;
    port_id += src_node_id;
    DEBUG("Port ID:  %ld -> %ld : %ld\n", src_node_id, dest_node_id, port_id);
    return port_id;
}

#if NETWORK_DELAY_TEST || !ENVIRONMENT_EC2
uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id, uint64_t send_thread_id)
{
    uint64_t port_id = 0;
    DEBUG("Calc port id %ld %ld %ld\n", src_node_id, dest_node_id, send_thread_id);
    port_id += g_total_node_cnt * dest_node_id;
    DEBUG("%ld\n", port_id);
    port_id += src_node_id;
    DEBUG("%ld\n", port_id);
    //  uint64_t max_send_thread_cnt = g_send_thread_cnt > g_client_send_thread_cnt ? g_send_thread_cnt : g_client_send_thread_cnt;
    //  port_id *= max_send_thread_cnt;
    port_id += send_thread_id * g_total_node_cnt * g_total_node_cnt;
    DEBUG("%ld\n", port_id);
    port_id = port_id % TPORT_WINDOW;
    port_id += TPORT_PORT;
    DEBUG("%ld\n", port_id);
    printf("Port ID:  %ld, %ld -> %ld : %ld\n", send_thread_id, src_node_id, dest_node_id, port_id);
    return port_id;
}
#else

uint64_t Transport::get_port_id(uint64_t src_node_id, uint64_t dest_node_id, uint64_t send_thread_id)
{
    uint64_t port_id = 0;
    DEBUG("Calc port id %ld %ld %ld\n", src_node_id, dest_node_id, send_thread_id);
    port_id += dest_node_id + src_node_id;
    DEBUG("%ld\n", port_id);
    port_id += send_thread_id * g_total_node_cnt * 2;
    DEBUG("%ld\n", port_id);
    port_id += TPORT_PORT;
    DEBUG("%ld\n", port_id);
    printf("Port ID:  %ld, %ld -> %ld : %ld\n", send_thread_id, src_node_id, dest_node_id, port_id);
    return port_id;
}
#endif

Socket *Transport::bind(uint64_t port_id)
{
    Socket *socket = get_socket();
    char socket_name[MAX_TPORT_NAME];
#if TPORT_TYPE == IPC
    sprintf(socket_name, "ipc://node_%ld.ipc", port_id);
#else
#if ENVIRONMENT_EC2
    sprintf(socket_name, "tcp://0.0.0.0:%ld", port_id);
    // sprintf(socket_name,"tcp://eth0:%ld",port_id);
#else
    sprintf(socket_name, "tcp://%s:%ld", ifaddr[g_node_id], port_id);
#endif
#endif
    printf("Sock Binding to %s %d\n", socket_name, g_node_id);
    int rc = socket->sock.bind(socket_name);
    if (rc < 0)
    {
        printf("Bind Error: %d %s\n", errno, strerror(errno));
        assert(false);
    }
    return socket;
}

Socket *Transport::connect(uint64_t dest_id, uint64_t port_id)
{
    Socket *socket = get_socket();
    char socket_name[MAX_TPORT_NAME];
#if TPORT_TYPE == IPC
    sprintf(socket_name, "ipc://node_%ld.ipc", port_id);
#else
#if ENVIRONMENT_EC2
    sprintf(socket_name, "tcp://%s;%s:%ld", ifaddr[g_node_id], ifaddr[dest_id], port_id);
    // sprintf(socket_name,"tcp://eth0;%s:%ld",ifaddr[dest_id],port_id);
#else
    sprintf(socket_name, "tcp://%s;%s:%ld", ifaddr[g_node_id], ifaddr[dest_id], port_id);
#endif
#endif
    printf("Sock Connecting to %s %d -> %ld\n", socket_name, g_node_id, dest_id);
    int rc = socket->sock.connect(socket_name);
    if (rc < 0)
    {
        printf("Connect Error: %d %s\n", errno, strerror(errno));
        assert(false);
    }
    return socket;
}

void Transport::init()
{
    client_input_threads = REM_THREAD_CNT_FOR_CLIENTS;
    /*
     * S, R number of seding and receving threads
     * Create S listening sockets for each node in the system, put them in queues for receiveing threads
     * Crate S sending sockets pair with dest_id and put them in hash map that keys are (node_id, thd_id) and value is socket
     */
    _sock_cnt = get_socket_count();

    rr = 0;
    printf("Tport Init %d: %ld\n", g_node_id, _sock_cnt);

    string path = get_path();
    read_ifconfig(path.c_str());

    for (uint64_t node_id = 0; node_id < g_total_node_cnt; node_id++)
    {
        if (node_id == g_node_id)
            continue;

        // Listening ports
        if (ISCLIENTN(node_id))
        {
            // for (uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++)
            for (uint64_t client_thread_id = 0; client_thread_id < g_client_send_thread_cnt; client_thread_id++)
            {
                uint64_t port_id = get_port_id(node_id, g_node_id, client_thread_id);
                Socket *sock = bind(port_id);
                if (ISCLIENT)
                {
                    recv_sockets.push_back(sock);
                }
                else
                {
                    recv_sockets_clients[node_id % client_input_threads].push_back(sock);
                }
                DEBUG("Socket insert: {%ld}: %ld\n", node_id, (uint64_t)sock);
                printf("client {%ld} Socket insert to %ld\n", node_id, node_id % client_input_threads);
            }
        }
        else
        {
            // for on SEND_THREAD_CNT
            for (uint64_t server_thread_id = 0; server_thread_id < g_send_thread_cnt; server_thread_id++)
            {
                uint64_t port_id = get_port_id(node_id, g_node_id, server_thread_id);
                Socket *sock = bind(port_id);
                // Sockets for clients and servers in different sets.
                if (ISCLIENT)
                {
                    recv_sockets.push_back(sock);
                }
                else
                {
                    recv_sockets_servers[node_id % (g_rem_thread_cnt - client_input_threads)].push_back(sock);
                }
                DEBUG("Socket insert: {%ld}: %ld\n", node_id, (uint64_t)sock);
                printf("server {%ld} Socket insert to %ld  sock:%ld\n", node_id, node_id % (g_rem_thread_cnt - client_input_threads), (uint64_t)sock);
                // printf("node_id:{%ld} g_rem_thread_cnt:%d  client_input_threads:%ld  \n", node_id,  g_rem_thread_cnt, client_input_threads);
            }
        }
        // Sending ports
        if (ISCLIENT)
        {
            // for (uint64_t client_thread_id = g_client_thread_cnt + g_client_rem_thread_cnt; client_thread_id < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt; client_thread_id++)
            for (uint64_t client_thread_id = 0; client_thread_id < g_client_send_thread_cnt; client_thread_id++)
            {
                uint64_t port_id = get_port_id(g_node_id, node_id, client_thread_id);
                std::pair<uint64_t, uint64_t> sender = std::make_pair(node_id, client_thread_id);
                Socket *sock = connect(node_id, port_id);
                send_sockets.insert(std::make_pair(sender, sock));
                DEBUG("Socket insert: {%ld,%ld}: %ld\n", node_id, client_thread_id, (uint64_t)sock);
            }
        }
        else
        {
            // for (uint64_t server_thread_id = g_thread_cnt + g_rem_thread_cnt; server_thread_id < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt; server_thread_id++)
            for (uint64_t server_thread_id = 0; server_thread_id < g_send_thread_cnt; server_thread_id++)
            {
                uint64_t port_id = get_port_id(g_node_id, node_id, server_thread_id);
                std::pair<uint64_t, uint64_t> sender = std::make_pair(node_id, server_thread_id);
                Socket *sock = connect(node_id, port_id);
                send_sockets.insert(std::make_pair(sender, sock));
                DEBUG("Socket insert: {%ld,%ld}: %ld\n", node_id, server_thread_id, (uint64_t)sock);
            }
        }
    }

    fflush(stdout);
}

// rename sid to send thread id
void Transport::send_msg(uint64_t send_thread_id, uint64_t dest_node_id, void *sbuf, int size)
{
    uint64_t abs_thd_id = send_thread_id - g_this_thread_cnt - g_this_rem_thread_cnt;

    uint64_t starttime = get_sys_clock();

    Socket *socket = send_sockets.find(std::make_pair(dest_node_id, abs_thd_id))->second;
    // Copy messages to nanomsg buffer
    DEBUG("%ld Sending batch of %d bytes to node %ld on socket %ld\n", send_thread_id, size, dest_node_id, (uint64_t)socket);
    INC_GLOB_STATS_ARR(bytes_sent, dest_node_id, size);

#if VIEW_CHANGES || LOCAL_FAULT
    bool failednode = false;

    if (ISSERVER)
    {
        uint64_t tid = send_thread_id % g_send_thread_cnt;
        stopMTX[tid].lock();
        for (uint i = 0; i < stop_nodes[tid].size(); i++)
        {
            if (dest_node_id == stop_nodes[tid][i])
            {
                failednode = true;
                break;
            }
        }
        stopMTX[tid].unlock();
    }
    else
    {
        clistopMTX.lock();
        for (uint i = 0; i < stop_replicas.size(); i++)
        {
            if (dest_node_id == stop_replicas[i])
            {
                failednode = true;
                break;
            }
        }
        clistopMTX.unlock();
    }

    if (!failednode)
    {

        // uint64_t ptr = 0;
        // RemReqType rtype;
        // ptr += sizeof(rtype);
        // ptr += sizeof(rtype);
        // ptr += sizeof(rtype);
        // memcpy(&rtype, &((char *)buf)[ptr], sizeof(rtype));
        // cout << rtype << endl;

        int rc = -1;
        uint64_t time = get_sys_clock();
        while ((rc < 0 && (get_sys_clock() - time < MSG_TIMEOUT || !simulation->is_setup_done())) && (!simulation->is_setup_done() || !simulation->is_done()))
        {
            rc = socket->sock.send(sbuf, size, NNG_FLAG_NONBLOCK);
        }
        if (rc < 0)
        {
            cout << "Adding failed node: " << dest_node_id << "\n";
            if (ISSERVER)
            {
                uint64_t tid = send_thread_id % g_send_thread_cnt;
                stopMTX[tid].lock();
                stop_nodes[tid].push_back(dest_node_id);
                stopMTX[tid].unlock();
            }
            else
            {
                clistopMTX.lock();
                stop_replicas.push_back(dest_node_id);
                clistopMTX.unlock();
            }
        }
    }
#else
    int rc = -1;
    while (rc < 0 && (!simulation->is_setup_done() || !simulation->is_done()))
    {
        rc = socket->sock.send(sbuf, size, NNG_FLAG_NONBLOCK);
    }
#endif

    // nn_freemsg(sbuf);
    DEBUG("%ld Batch of %d bytes sent to node %ld\n", send_thread_id, size, dest_node_id);

    INC_STATS(send_thread_id, msg_send_time, get_sys_clock() - starttime);
    INC_STATS(send_thread_id, msg_send_cnt, 1);
}

// Listens to sockets for messages from other nodes
std::vector<Message *> *Transport::recv_msg(uint64_t thd_id)
{
    uint64_t abs_thd_id = thd_id - g_this_thread_cnt;

    int bytes = 0;
    void *buf = NULL;
    std::vector<Message *> *msgs = NULL;

    uint64_t ctr, start_ctr;
    uint64_t starttime = get_sys_clock();
    if (!ISSERVER)
    {
        uint64_t rand = (starttime % recv_sockets.size()) / g_this_rem_thread_cnt;
        ctr = abs_thd_id;

        if (ctr >= recv_sockets.size())
            return msgs;
        if (g_this_rem_thread_cnt < g_total_node_cnt)
        {
            ctr += rand * g_this_rem_thread_cnt;
            while (ctr >= recv_sockets.size())
            {
                ctr -= g_this_rem_thread_cnt;
            }
        }
        assert(ctr < recv_sockets.size());
    }
    else
    {
        // One thread manages client sockets, while others handles server sockets.
        if (abs_thd_id < client_input_threads)
        {
            ctr = get_next_socket(thd_id, recv_sockets_clients[abs_thd_id].size());
        }
        else
        {
            ctr = get_next_socket(thd_id, recv_sockets_servers[abs_thd_id - client_input_threads].size());
        }
    }
    start_ctr = ctr;

    while (bytes <= 0 && (!simulation->is_setup_done() || (simulation->is_setup_done() && !simulation->is_done())))
    {
        Socket *socket;
        if (!ISSERVER)
        {
            socket = recv_sockets[ctr];
            bytes = socket->sock.recv(&buf, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK);

            ctr = (ctr + g_this_rem_thread_cnt);

            if (ctr >= recv_sockets.size())
                ctr = (abs_thd_id) % recv_sockets.size();
            if (ctr == start_ctr)
                break;
        }
        else
        { // Only servers.
            if (abs_thd_id < client_input_threads)
            {
                socket = recv_sockets_clients[abs_thd_id][ctr];
            }
            else
            {
                socket = recv_sockets_servers[abs_thd_id - client_input_threads][ctr];
            }
            bytes = socket->sock.recv(&buf, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK);
        }
        if (bytes > 0)
        {
            break;
        }
        else
        {
            if (ISSERVER)
            {
                if (thd_id < client_input_threads)
                {
                    ctr = get_next_socket(thd_id, recv_sockets_clients[abs_thd_id].size());
                }
                else
                {

                    ctr = get_next_socket(thd_id, recv_sockets_servers[abs_thd_id - client_input_threads].size());
                }
                if (ctr == start_ctr)
                    break;
            }
        }
    }

    if (bytes <= 0)
    {
        INC_STATS(thd_id, msg_recv_idle_time, get_sys_clock() - starttime);
        return msgs;
    }

    INC_STATS(thd_id, msg_recv_time, get_sys_clock() - starttime);
    INC_STATS(thd_id, msg_recv_cnt, 1);

    starttime = get_sys_clock();

    msgs = Message::create_messages((char *)buf);
    DEBUG("Batch of %d bytes recv from node %ld; Time: %f\n", bytes, msgs->front()->return_node_id, simulation->seconds_from_start(get_sys_clock()));

    nn::freemsg(buf, bytes);

    return msgs;
}
