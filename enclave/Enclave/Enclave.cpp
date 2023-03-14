#include <stdbool.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sgx_tkey_exchange.h"
#include <sgx_trts.h>
#include "Enclave_t.h"
#include "sgx_tseal.h"
#include "sgx_tcrypto.h"
#include "string.h"
#include "sgx_trts.h"

uint32_t num = -1;		   //sequence number primary asks
uint32_t prepare_num = -1; //prepare counter which replicas use
uint32_t commit_num = -1; //commit counter which replicas use
uint32_t execute_num = -1; //execute counter which replicas / primary use
sgx_ecc_state_handle_t ctx;
sgx_ra_context_t context = 50;
sgx_ec256_private_t p_private;
sgx_ec256_public_t p_public;
sgx_ec_key_128bit_t mk_key;

//Struct which will be sealed and saved to disk
//it contains the private and public key of a ECDSA session
typedef struct tesv_sealed_data
{
	sgx_ec256_private_t p_private;
	sgx_ec256_public_t p_public;
} esv_sealed_data_t;

//Seal the data and write in the file: sealed_data_file_name
int esv_seal_keys(const char *sealed_data_file_name)
{
	sgx_status_t ret = SGX_ERROR_INVALID_PARAMETER;
	sgx_sealed_data_t *sealed_data = NULL;
	uint32_t sealed_size = 0;

	esv_sealed_data_t data;
	data.p_private = p_private;
	data.p_public = p_public;
	size_t data_size = sizeof(data);

	sealed_size = sgx_calc_sealed_data_size(NULL, data_size);
	if (sealed_size != 0)
	{
		ocall_print("Sealed size != 0");
		sealed_data = (sgx_sealed_data_t *)malloc(sealed_size);
		ret = sgx_seal_data(NULL, NULL, data_size, (uint8_t *)&data, sealed_size, sealed_data);
		if (ret == SGX_SUCCESS)
			esv_write_data(sealed_data_file_name, (unsigned char *)sealed_data, sealed_size);
		else
			free(sealed_data);
	}
	return ret;
}

sgx_status_t enclave_init_ra(int b_pse, sgx_ra_context_t p_context)
{
	// isv enclave call to trusted key exchange library.
	sgx_status_t ret;
#ifdef SUPPLIED_KEY_DERIVATION
	ret = sgx_ra_init_ex(&mk_key, b_pse, key_derivation, p_context);
#else
	strlcpy((char *)&mk_key, "aabbccdd", 8);
	// ret = sgx_ra_get_keys(p_context, SGX_RA_KEY_MK, &mk_key);
#endif
	return ret;
}

//Init ECDSA context, load the file from disk or create a new key pair
int esv_init(const char *sealed_data_file_name)
{
	sgx_status_t ret = SGX_ERROR_INVALID_PARAMETER;
	esv_sealed_data_t *unsealed_data = NULL;
	sgx_sealed_data_t *enc_data = NULL;
	size_t enc_data_size;
	uint32_t dec_size = 0;

	// ocall_print((char *)&s_key);
	ret = enclave_init_ra(false, context);

	//open an ECDSA context
	ret = sgx_ecc256_open_context(&ctx);
	if (ret != SGX_SUCCESS)
		return ret;
	if (sealed_data_file_name != NULL)
	{
		esv_read_data(sealed_data_file_name, (unsigned char **)&enc_data, &enc_data_size);
		dec_size = sgx_get_encrypt_txt_len(enc_data);
		if (dec_size != 0)
		{
			unsealed_data = (esv_sealed_data_t *)malloc(dec_size);
			sgx_sealed_data_t *tmp = (sgx_sealed_data_t *)malloc(enc_data_size);
			//copy the data from 'esv_read_data' to trusted enclave memory. This is needed because otherwise sgx_unseal_data will rise an error
			memcpy(tmp, enc_data, enc_data_size);
			ret = sgx_unseal_data(tmp, NULL, NULL, (uint8_t *)unsealed_data, &dec_size);
			if (ret != SGX_SUCCESS)
				return ret;
			p_private = unsealed_data->p_private;
			p_public = unsealed_data->p_public;
		}
	}
	else
	{
		ret = sgx_ecc256_create_key_pair(&p_private, &p_public, ctx);
		esv_seal_keys("sealeddata");
		// ocall_print((char *)&p_private);
		// ocall_print((char *)&p_public);
	}
	return ret;
}

//This function signs a given message and returns on success the signature object
//DEBUG by writing the signature on a file using the commented code
int esv_sign(const char *message, uint32_t *signature, size_t sig_len)
{
	sgx_status_t ret = SGX_ERROR_INVALID_PARAMETER;
	const size_t MAX_MESSAGE_LENGTH = 255;
	sgx_ec256_signature_t res;
	
	ret = sgx_ecdsa_sign((uint8_t *)"hello darkness my old friend", strnlen(message, MAX_MESSAGE_LENGTH), &p_private, &res, ctx);
	if (ret != SGX_SUCCESS)
	{
		ocall_print("Sign Error");
		return ret;
	}
	// Copy to half of ecc sig to a single array
	memcpy(signature, res.x, sig_len / 2);
	memcpy(signature + (sig_len / 2 / sizeof(uint32_t)), res.y, sig_len / 2);
	return ret;
}

//This function verifies a given message with its signature object and returns on success SGX_EC_VALID or on failure SGX_EC_INVALID_SIGNATURE
int esv_verify(const char *message, void *signature, size_t sig_len)
{
	sgx_status_t ret = SGX_ERROR_INVALID_PARAMETER;
	const size_t MAX_MESSAGE_LENGTH = 255;
	uint8_t res;
	sgx_ec256_signature_t *sig = (sgx_ec256_signature_t *)signature;
	ret = sgx_ecdsa_verify((uint8_t *)message, strnlen(message, MAX_MESSAGE_LENGTH), &p_public, sig, &res, ctx);
	return res;
}

int generate_random_number()
{
	ocall_print("Processing random number generation...");
	return 42;
}

uint64_t generate_unique_sequence_number(const char *hash)
{
	num++;
	return num;
}

uint64_t get_prepare_counter(const char *hash)
{
	prepare_num++;
	return prepare_num;
}

uint64_t get_commit_counter(const char *hash)
{
	commit_num++;
	return commit_num;
}

uint64_t get_execute_counter(const char *hash)
{
	execute_num++;
	return execute_num;
}

//This function close the ECDSA context
int esv_close()
{
	sgx_status_t ret = sgx_ecc256_close_context(ctx);
	return ret;
}

int cmac_sign(const char *message, uint8_t *signature, size_t src_len)
{
	uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};
	int ret = sgx_rijndael128_cmac_msg(&mk_key, (uint8_t *)&num, (uint32_t)src_len, &mac);
	memcpy(signature, &mac, SGX_CMAC_MAC_SIZE);
	return ret;
}

int cmac_verify(const char *message, void *signature, size_t src_len)
{
	sgx_status_t ret = SGX_ERROR_OUT_OF_MEMORY;
	uint8_t mac[SGX_CMAC_MAC_SIZE] = {0};
	ret = sgx_rijndael128_cmac_msg(&mk_key, (uint8_t *)message, (uint32_t)src_len, &mac);
	return 1;
}