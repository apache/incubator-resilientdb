#include <stdio.h>
#include <iostream>
#include <time.h>
#include <string>
#include <sstream>
#include <time.h>
#include <chrono>
#include <ctime>
#include <ratio>
#include <nng/nng.h>
#include <nng/protocol/pair0/pair.h>

using namespace std;

#define TEE_SEQUENCE_COUNTER 0
#define TEE_VERIFY 1
#define TEE_PREPARE_COUNTER 2
#define TEE_EXECUTE_COUNTER 3
#define TEE_TERMINATE 10
#define TEE_SIGN_INVALID -1

struct enclave_message {
    uint64_t type;
    uint64_t counter;
    uint32_t signature[16]; //Signature size: check
};

nng_socket connect_to_enclave(const char* url){
    nng_socket sock;
    int rv = -1;
    
    if(rv = nng_pair0_open(&sock) != 0){
        cout << nng_strerror(rv);
        fflush(stdout);
    }
    if(rv = nng_dial(sock, url, NULL, 0)){
        cout << nng_strerror(rv);
        fflush(stdout);
    }
    nng_setopt_ms(sock, NNG_OPT_RECVTIMEO, 100);
    return sock;
}

uint64_t get_server_clock(){
    unsigned hi, lo;
	__asm__ __volatile__("rdtsc"
						 : "=a"(lo), "=d"(hi));
	uint64_t ret = ((uint64_t)lo) | (((uint64_t)hi) << 32);
	ret = (uint64_t)((double)ret / 2.5);
    return ret;
}

int main(){
    nng_socket enclave_sock = connect_to_enclave("ipc:///tmp/sgx_seq.ipc");
    cout << "Connected" << endl;
    struct enclave_message temp;
    struct enclave_message* recv_buf;
    size_t sz;

    temp.type = TEE_TERMINATE;
    temp.counter = 12;
    if(nng_send(enclave_sock, &temp, sizeof(struct enclave_message), 0) != 0){
        cout << "nng_send failed" << endl;
    }
    while(nng_recv(enclave_sock, &recv_buf, &sz, NNG_FLAG_ALLOC) < 0){
        continue;
    }
    printf("Received Message of type %ld for counter %ld\n", recv_buf->type, recv_buf->type);
    //Ask for verifcation
    // temp.type = TEE_VERIFY;
    // temp.counter = recv_buf->counter;
    // for(int i = 0; i < 16; i++){
    //     cout << recv_buf->signature[i] << endl;
    //     temp.signature[i] = recv_buf->signature[i];
    // }
    // cout << endl;
    // auto start = get_server_clock();
    // nng_send(enclave_sock, &temp, sizeof(struct enclave_message), 0);
    // while(nng_recv(enclave_sock, &recv_buf, &sz, NNG_FLAG_ALLOC) < 0){
    //     continue;
    // }
    // cout << "Recieved type: " << temp.type << endl;
    // fflush(stdout);

    return 0;
}