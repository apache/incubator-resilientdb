#include <stdio.h>
#include <iostream>
#include <time.h>
#include <string>
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>

#include <time.h>
#include <chrono>
#include <thread>
#include <mutex>
#include "Enclave_u.h"
#include "sgx_urts.h"
#include "sgx_utils/sgx_utils.h"

using namespace std;

#define fopen_s(pFile, filename, mode) ((*(pFile)) = fopen((filename), (mode))) == NULL
#define TEE_SEQUENCE_COUNTER 0
#define TEE_VERIFY 1
#define TEE_PREPARE_COUNTER 2
#define TEE_COMMIT_COUNTER 3
#define TEE_EXECUTE_COUNTER 4
#define TEE_TERMINATE 10
#define TEE_SIGN_INVALID -1
#define ECC_SIG_SIZE 16

struct enclave_message
{
    uint64_t type;
    uint64_t counter;
    uint32_t signature[ECC_SIG_SIZE]; //Signature size: check
};

struct ptr_msg
{
    int counter;
    uint8_t sign[SGX_CMAC_MAC_SIZE];
};
/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;
sgx_ec256_public_t pub_key;
nng_socket seq_number_sock, prepare_sock, commit_sock, exec_sock;
// mutex seq_mtx, prepare_mtx, commit_mtx, exec_mtx;
mutex mtx;
bool running = true;

char *data_buffer = (char *)malloc(SGX_CMAC_MAC_SIZE);
//Reads a file from file system and allocates memory for it and stores the data in pp_data
long readFromFile(const char *file_name, unsigned char **pp_data)
{
    FILE *infile;
    int err;
    long fsize = 0;
    err = fopen_s(&infile, file_name, "rb");
    if (err == 0)
    {
        fseek(infile, 0L, SEEK_END);
        fsize = ftell(infile);
        rewind(infile);
        *pp_data = (unsigned char *)calloc(fsize, sizeof(unsigned char));
        unsigned char *tmp = *pp_data;
        size_t len = fread(tmp, sizeof(unsigned char), fsize, infile);
        fclose(infile);
    }
    else
    {
        printf("Failed to open File %s", file_name);
    }
    return fsize;
}
//Write data from p_data to file system
void writeToFile(const char *file_name, const unsigned char *p_data, size_t len)
{
    FILE *outfile;
    int err;
    err = fopen_s(&outfile, file_name, "wb");
    if (err == 0)
    {
        for (int i = 0; i < len; i++)
        {
            fputc(p_data[i], outfile);
        }
        fclose(outfile);
    }
    else
    {
        printf("Failed to open File %s", file_name);
    }
}
//Ocall (will be executed within the enclave)
void esv_write_data(const char *file_name, const unsigned char *p_data, size_t len)
{
    writeToFile(file_name, p_data, len);
}
//Ocall (will be executed within the enclave)
void esv_read_data(const char *file_name, unsigned char **pp_data, size_t *len)
{
    *len = readFromFile(file_name, pp_data);
}
// OCall implementations
void ocall_print(const char *str)
{
    printf("%s\n", str);
}
// OCall implementations
void ocall_print_int(uint32_t number)
{
    printf("%u\n", number);
}

nng_socket create_sock_and_bind(const char *url)
{

    nng_socket sock;
    int rv;
    if (rv = nng_pair0_open(&sock) != 0)
    {
        cout << nng_strerror(rv);
        fflush(stdout);
    }
    if (rv = nng_listen(sock, url, NULL, 0))
    {
        cout << nng_strerror(rv);
        fflush(stdout);
    }
    return sock;
}

uint64_t get_server_clock()
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc"
                         : "=a"(lo), "=d"(hi));
    uint64_t ret = ((uint64_t)lo) | (((uint64_t)hi) << 32);
    ret = (uint64_t)((double)ret / 3);
    return ret;
}

void sign_and_send_number(uint64_t number, nng_socket sock, uint64_t type)
{
    int ret, res = -1;
    char message[32] = {0};
    uint32_t signature[ECC_SIG_SIZE] = {0};
    struct enclave_message send_msg;

    send_msg.type = type;
    sprintf(message, "%ld", number);
    ret = esv_sign(global_eid, &res, message, signature, ECC_SIG_SIZE * sizeof(uint32_t));
    if (ret != SGX_SUCCESS)
    {
        cout << "Error Code: " << ret << endl;
        exit(1);
    }
    memcpy(send_msg.signature, signature, ECC_SIG_SIZE * sizeof(uint32_t));
    // for (int i = 0; i < ECC_SIG_SIZE; i++)
    // {
    //     printf("%u\n", send_msg.signature[i]);
    // }
    send_msg.counter = number;
    nng_send(sock, &send_msg, sizeof(struct enclave_message), 0);
}

void application_(nng_socket sock, int thread_id)
{
    while (running)
    {
        struct enclave_message *buf;
        size_t sz;
        int res = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC);
        if (res == 0)
        {
            switch (buf->type)
            {
            case TEE_TERMINATE:
            {
                mtx.lock();
                running = false;
                uint64_t ack = 0;
                sign_and_send_number(ack, seq_number_sock, buf->type);
                nng_close(seq_number_sock);
                nng_close(prepare_sock);
                nng_close(commit_sock);
                nng_close(exec_sock);
                mtx.unlock();
                // cout << "Terminate " << running << endl;
                break;
            }
            case TEE_SEQUENCE_COUNTER:
            {
                uint64_t sequence_number = 0;

                mtx.lock();
                sgx_status_t status = generate_unique_sequence_number(global_eid, &sequence_number, NULL);
                // cout << "Sign Sequence Number: " << sequence_number << endl;
                sign_and_send_number(sequence_number, seq_number_sock, buf->type);
                mtx.unlock();
                break;
            }

            case TEE_VERIFY:
                //TODO
                break;

            case TEE_PREPARE_COUNTER:
            {
                uint64_t prepare_counter = 0;

                mtx.lock();
                sgx_status_t status = get_prepare_counter(global_eid, &prepare_counter, NULL);
                // cout << "Sign Prepare Counter: " << prepare_counter << endl;
                sign_and_send_number(prepare_counter, prepare_sock, buf->type);
                mtx.unlock();
                break;
            }
            case TEE_COMMIT_COUNTER:
            {
                uint64_t commit_counter = 0;

                mtx.lock();
                sgx_status_t status = get_commit_counter(global_eid, &commit_counter, NULL);
                // cout << "Sign Commit Counter: " << commit_counter << endl;
                sign_and_send_number(commit_counter, commit_sock, buf->type);
                mtx.unlock();
                break;
            }
            case TEE_EXECUTE_COUNTER:
            {
                uint64_t exec_counter = 0;

                mtx.lock();
                sgx_status_t status = get_execute_counter(global_eid, &exec_counter, NULL);
                // cout << "Sign EXEC Counter: " << exec_counter << endl;
                sign_and_send_number(exec_counter, exec_sock, buf->type);
                mtx.unlock();
                break;
            }
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    int res = -1;
    // global_sock = create_sock_and_bind("tcp://127.0.0.1:5000");
    seq_number_sock = create_sock_and_bind("ipc:///tmp/sgx_seq.ipc");
    prepare_sock = create_sock_and_bind("ipc:///tmp/sgx_prepare.ipc");
    commit_sock = create_sock_and_bind("ipc:///tmp/sgx_commit.ipc");
    exec_sock = create_sock_and_bind("ipc:///tmp/sgx_exec.ipc");
    std::cout << "Enclave Ready Version: 1.5:2110" << std::endl;
    if (initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so") < 0)
    {
        std::cout << "Fail to initialize enclave." << std::endl;
        return 1;
    }

    //Initialize the keys p_private and p_public
    esv_init(global_eid, &res, NULL);
    thread t1(application_, seq_number_sock, 1);
    thread t2(application_, prepare_sock, 2);
    thread t3(application_, commit_sock, 3);
    thread t4(application_, exec_sock, 4);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    esv_close(global_eid, &res);
    return 0;
}
