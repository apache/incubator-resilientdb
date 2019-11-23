#ifndef CRYPTO_H
#define CRYPTO_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wextra"

#include <hex.h>
#include <osrng.h>
#include <pssr.h>
#include <rsa.h>
#include <whrlpool.h>
#pragma GCC diagnostic pop

#include <string>
//#include <vector>
//#include <memory>

//New libraries for AES
#include <iomanip>
#include "modes.h"
#include "aes.h"
#include "filters.h"

//New libraries for ED25519
#include "xed25519.h"
using CryptoPP::ArraySink;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::ed25519;
using CryptoPP::ed25519PrivateKey;
using CryptoPP::ed25519PublicKey;
using CryptoPP::ed25519Signer;
using CryptoPP::ed25519Verifier;
using CryptoPP::NullRNG;
using CryptoPP::SignerFilter;
using CryptoPP::StringSink;
using CryptoPP::StringSource;

typedef unsigned char byte;

/*                                  *
 *	       ED25519 algorithm    *
 *                                  */

//extern std::vector<std::shared_ptr<CryptoPP::CMAC<CryptoPP::AES>>> CMACsend;
//extern std::vector<std::shared_ptr<CryptoPP::CMAC<CryptoPP::AES>>> CMACrecv;

// Initialize the signer and the keys for ED25519
inline void ED25519GenerateKeys(string &skey, string &pkey)
{
    AutoSeededRandomPool prng;
    // Initilize singer;
    signer.AccessPrivateKey().GenerateRandom(prng);
    const ed25519PrivateKey &privKey = dynamic_cast<const ed25519PrivateKey &>(signer.GetPrivateKey());

    // Initialize keys just in case we need them
    const byte *secretKeyByte = privKey.GetPrivateKeyBytePtr();
    const byte *publicKeyByte = privKey.GetPublicKeyBytePtr();
    skey.clear();
    pkey.clear();
    for (uint64_t f = 0; f < ed25519PrivateKey::SECRET_KEYLENGTH; f++)
    {
        skey.push_back(secretKeyByte[f]);
    }

    for (uint64_t f = 0; f < ed25519PrivateKey::PUBLIC_KEYLENGTH; f++)
    {
        pkey.push_back(publicKeyByte[f]);
    }
}

inline bool ed25519PrivateKey::Validate(RandomNumberGenerator &rng, unsigned int level) const
{
    CRYPTOPP_UNUSED(rng);

    if (level >= 1 && IsSmallOrder(m_pk) == true)
        return false;
    if (level >= 3)
    {
        // Verify m_pk is pairwise consistent with m_sk
        SecByteBlock pk(PUBLIC_KEYLENGTH);
        SecretToPublicKey(pk, m_sk);

        if (VerifyBufsEqual(pk, m_pk, PUBLIC_KEYLENGTH) == false)
            return false;
    }

    return true;
}

inline string ED25519signString(const string message)
{
    AutoSeededRandomPool prng;
    string signature;
    /*size_t siglen = signer.MaxSignatureLength();
    signature.resize(siglen);
    // Sign, and trim signature to actual size
    siglen = signer.SignMessage(prng, (const byte*)&message[0], message.size(), (byte*)&signature[0]);
    signature.resize(siglen);*/

    StringSource(message, true, new SignerFilter(NullRNG(), signer, new StringSink(signature)));

    return signature;
}

inline bool ED25519verifyString(const string message, const string signature, const uint64_t receiver_node_id)
{
    bool valid = true;
    ed25519::Verifier thisVerifier = verifier[receiver_node_id];
    CryptoPP::StringSource(signature + message, true,
                           new CryptoPP::SignatureVerificationFilter(thisVerifier,
                                                                     new CryptoPP::ArraySink((byte *)&valid, sizeof(valid))));
    if (valid == false)
    {
        assert(0);
    }
    return valid;
}

/*                              *
 *	       RSA algorithm    *
 *                              */
//code from http://marko-editor.com/articles/cryptopp_sign_string/
// see http://www.cryptopp.com/wiki/RSA
using Signer = CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Signer;
using Verifier = CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Verifier;

//==============================================================================
// Return a KeyPairHex for each node. This function is used by the nodes to obtain their keyPair
inline KeyPairHex RsaGenerateHexKeyPair(unsigned int aKeySize)
{
    KeyPairHex keyPair;

    // PGP Random Pool-like generator
    CryptoPP::AutoSeededRandomPool rng;

    // generate keys
    CryptoPP::RSA::PrivateKey privateKey;
    privateKey.GenerateRandomWithKeySize(rng, aKeySize);
    CryptoPP::RSA::PublicKey publicKey(privateKey);

    // save keys
    publicKey.Save(CryptoPP::HexEncoder(
                       new CryptoPP::StringSink(keyPair.publicKey))
                       .Ref());
    privateKey.Save(CryptoPP::HexEncoder(
                        new CryptoPP::StringSink(keyPair.privateKey))
                        .Ref());

    return keyPair;
}

//==============================================================================
inline std::string RsaSignString(const std::string &aPrivateKeyStrHex,
                                 const std::string &aMessage)
{

    // decode and load private key (using pipeline)
    CryptoPP::RSA::PrivateKey privateKey;
    privateKey.Load(CryptoPP::StringSource(aPrivateKeyStrHex, true,
                                           new CryptoPP::HexDecoder())
                        .Ref());

    // sign message
    std::string signature;
    Signer signer(privateKey);
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::StringSource ss(aMessage, true,
                              new CryptoPP::SignerFilter(rng, signer,
                                                         new CryptoPP::HexEncoder(
                                                             new CryptoPP::StringSink(signature))));

    return signature;
}

//==============================================================================
inline bool RsaVerifyString(const std::string &aPublicKeyStrHex,
                            const std::string &aMessage,
                            const std::string &aSignatureStrHex)
{

    // decode and load public key (using pipeline)
    CryptoPP::RSA::PublicKey publicKey;
    publicKey.Load(CryptoPP::StringSource(aPublicKeyStrHex, true,
                                          new CryptoPP::HexDecoder())
                       .Ref());

    // decode signature
    std::string decodedSignature;
    CryptoPP::StringSource ss(aSignatureStrHex, true,
                              new CryptoPP::HexDecoder(
                                  new CryptoPP::StringSink(decodedSignature)));

    // verify message
    bool result = false;
    Verifier verifier(publicKey);
    CryptoPP::StringSource ss2(decodedSignature + aMessage, true,
                               new CryptoPP::SignatureVerificationFilter(verifier,
                                                                         new CryptoPP::ArraySink((byte *)&result,
                                                                                                 sizeof(result))));

    return result;
}

/*                                    *
 *	      CMAC-AES algorithm          *
 *                                    */

#include "osrng.h"
using CryptoPP::AutoSeededRandomPool;

#include "cryptlib.h"
using CryptoPP::Exception;

#include "cmac.h"
using CryptoPP::CMAC;

#include "aes.h"
using CryptoPP::AES;

#include "sha.h"
using CryptoPP::SHA1;

#include "hex.h"
using CryptoPP::HexDecoder;
using CryptoPP::HexEncoder;

#include "filters.h"
using CryptoPP::HashFilter;
using CryptoPP::HashVerificationFilter;
using CryptoPP::SignatureVerificationFilter;
using CryptoPP::SignerFilter;
using CryptoPP::StringSink;
using CryptoPP::StringSource;

#include "secblock.h"
using CryptoPP::SecByteBlock;

#include "rsa.h"
using CryptoPP::InvertibleRSAFunction;
using CryptoPP::RSA;
using CryptoPP::RSASS;

#include "pssr.h"
using CryptoPP::PSS;

#include "sha.h"
using CryptoPP::SHA1;

#include "files.h"
using CryptoPP::FileSink;
using CryptoPP::FileSource;

#include <whrlpool.h>

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
//#include <memory>

//CMAC-AES key generator
//Return a keyPairHex where just the priv key is set
inline KeyPairHex CmacGenerateHexKey2(unsigned int aKeySize)
{
    KeyPairHex keyPair;
    keyPair.publicKey = "";
    keyPair.privateKey = "";
    AutoSeededRandomPool prng;
    //int keySize = 16 ; // AES::DEFAULT_KEYLENGTH
    SecByteBlock key(aKeySize);
    prng.GenerateBlock(key.data(), key.size()); // Key is a SecByteBlock that contains the key.

    StringSource ss(key.data(), key.size(), true, new HexEncoder(new StringSink(keyPair.privateKey)) // HexEncoder
    );                                                                                               // StringSource

    return keyPair;
}

//inline std::shared_ptr<CryptoPP::CMAC<AES>> CMACgenerateInstance(const std::string &key) {
//    SecByteBlock byteKey((const unsigned char *)key.data(), key.size());
//    std::shared_ptr<CMAC<AES>> cmac(new CMAC<AES>(byteKey.data(), byteKey.size()));
//    std::cout << "USING GLOBAL CMAC instances" << std::endl;
//    return cmac;
//}

//CMAC-AES key generator
//Return a keyPairHex where just the priv key is set
inline string CmacGenerateHexKey(unsigned int aKeySize)
{
    string privKey;
    AutoSeededRandomPool prng;
    //int keySize = 16 ; // AES::DEFAULT_KEYLENGTH
    SecByteBlock key(aKeySize);
    prng.GenerateBlock(key.data(), key.size()); // Key is a SecByteBlock that contains the key.

    StringSource ss(key.data(), key.size(), true, new HexEncoder(new StringSink(privKey)) // HexEncoder
    );                                                                                    // StringSource
    return privKey;
}

//inline string CMACsignWithMAC(std::shared_ptr<CryptoPP::CMAC<CryptoPP::AES>> &cmac, std::string &message) {
//    std::string mac;
//    StringSource(message, true, new HashFilter(*(cmac.get()), new StringSink(mac)));
//    return mac;
//}

// CMAC-AES Signature generator
// Return a token, mac, to verify the a message.
inline string CmacSignString(const std::string &aPrivateKeyStrHex,
                             const string &aMessage)
{

    fflush(stdout);
    std::string mac = "";

    //KEY TRANSFORMATION. https://stackoverflow.com/questions/26145776/string-to-secbyteblock-conversion

    SecByteBlock privKey((const unsigned char *)(aPrivateKeyStrHex.data()), aPrivateKeyStrHex.size());

    CMAC<AES> cmac(privKey.data(), privKey.size());

    StringSource ss1(aMessage, true, new HashFilter(cmac, new StringSink(mac)));

    return mac;
}

//inline bool CMACverifyWithMAC(std::shared_ptr<CryptoPP::CMAC<CryptoPP::AES>> &cmac, std::string &message, std::string &mac) {
//    bool res = true;
//    const int flags = HashVerificationFilter::THROW_EXCEPTION | HashVerificationFilter::HASH_AT_END;
//    try {
//        StringSource(message + mac, true, new HashVerificationFilter(*(cmac.get()), NULL, flags)); // StringSource
//    } catch(std::exception &e) {
//        res = false;
//    }
//
//    return res;
//}
// CMAC-AES Verification function
// This function should return a bool but it will return always true. On the other hand HashVerificationFilter will trought an
// error if something fails.
// Task: Adjust HashVerificationFilter to set a true or false value.
inline bool CmacVerifyString(const std::string &aPublicKeyStrHex,
                             const string &aMessage,
                             const string &mac)
{
    bool res = true;

    fflush(stdout);
    // KEY TRANSFORMATION
    //https://stackoverflow.com/questions/26145776/string-to-secbyteblock-conversion
    SecByteBlock privKey((const unsigned char *)(aPublicKeyStrHex.data()), aPublicKeyStrHex.size());
    CMAC<AES> cmac(privKey.data(), privKey.size());
    const int flags = HashVerificationFilter::THROW_EXCEPTION | HashVerificationFilter::HASH_AT_END;

    // MESSAGE VERIFICATION
    //StringSource::StringSource(const char * string, bool pumpAll, BufferedTransformation * attachment = NULL)
    StringSource ss3(aMessage + mac, true, new HashVerificationFilter(cmac, NULL, flags)); // StringSource

    return res;
}

inline void signingClientNode(string message, string &signature, string &pkey, uint64_t dest_node)
{
#if CRYPTO_METHOD_RSA
    signature = RsaSignString(g_priv_key, message);
    pkey = g_pub_keys[g_node_id];
#elif CRYPTO_METHOD_ED25519
    signature = ED25519signString(message);
    pkey = g_pub_keys[g_node_id];
#elif CRYPTO_METHOD_CMAC_AES
    signature = CmacSignString(cmacPrivateKeys[dest_node], message);
    pkey = cmacPrivateKeys[dest_node];
#endif
}

inline void signingNodeNode(string message, string &signature, string &pkey, uint64_t dest_node)
{
#if CRYPTO_METHOD_CMAC_AES
    signature = CmacSignString(cmacPrivateKeys[dest_node], message);
    pkey = cmacPrivateKeys[dest_node];
#elif CRYPTO_METHOD_ED25519
    signature = ED25519signString(message);
    pkey = g_pub_keys[g_node_id];
#elif CRYPTO_METHOD_RSA
    signature = RsaSignString(g_priv_key, message);
    pkey = g_pub_keys[g_node_id];
#endif
}

inline bool validateClientNode(string message, string pubKey, string signature, uint64_t return_node_id)
{
#if CRYPTO_METHOD_RSA
    return RsaVerifyString(pubKey, message, signature);
#elif CRYPTO_METHOD_ED25519
    return ED25519verifyString(message, signature, return_node_id);
#elif CRYPTO_METHOD_CMAC_AES
    return CmacVerifyString(pubKey, message, signature);
#endif
    return true;
}

inline bool validateNodeNode(string message, string pubKey, string signature, uint64_t return_node_id)
{
#if CRYPTO_METHOD_CMAC_AES
    return CmacVerifyString(pubKey, message, signature);
    //return CMACverifyWithMAC(CMACrecv[return_node_id], message, signature);
#elif CRYPTO_METHOD_ED25519
    return ED25519verifyString(message, signature, return_node_id);
#elif CRYPTO_METHOD_RSA
    return RsaVerifyString(pubKey, message, signature);
#endif
    return true;
}

#if CRYPTO_METHOD_CMAC_AES
inline string getCmacRequiredKey(uint64_t dest_node)
{
    return cmacPrivateKeys[dest_node];
}
#endif

inline string getOtherRequiredKey(uint64_t dest_node)
{
#if CRYPTO_METHOD_ED25519
    return g_pub_keys[g_node_id];
#elif CRYPTO_METHOD_RSA
    return g_pub_keys[g_node_id];
#endif
}

inline string getsignNodeNode(string message, uint64_t dest_node)
{
#if CRYPTO_METHOD_CMAC_AES
    //return CMACsignWithMAC(CMACsend[dest_node], message);
    return CmacSignString(cmacPrivateKeys[dest_node], message);
#elif CRYPTO_METHOD_ED25519
    return ED25519signString(message);
#elif CRYPTO_METHOD_RSA
    return RsaSignString(g_priv_key, message);
#endif
}

#endif // CRYPTO_H
