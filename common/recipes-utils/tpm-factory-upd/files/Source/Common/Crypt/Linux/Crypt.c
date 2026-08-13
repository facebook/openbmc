/**
 *  @brief      Implements the Cryptography interface for Linux
 *  @details    This module provides cryptographic functions.
 *  @file       Linux\Crypt.c
 *
 *  Copyright 2016 - 2025 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/// The used CRC-32 polynomial in hexadecimal representation
#define CRC32MASKREV 0xEDB88320
/// OpenSSL API functionality differs depending on version
#define OPENSSL_VERSION_1_1_1   0x10101000L
#define OPENSSL_VERSION_3_0     0x30000000L

#include "Crypt.h"

#include <string.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_3_0
#include <openssl/param_build.h>
#endif

#if OPENSSL_VERSION_NUMBER < OPENSSL_VERSION_1_1_1
#pragma message OPENSSL_VERSION_TEXT
#pragma GCC error "openssl version < 1.1.1 no longer supported"
#endif

/**
 *  @brief      Calculate HMAC-SHA-1 on the given message
 *  @details    This function calculates a HMAC-SHA-1 on the input message.
 *
 *  @param      PrgbInputMessage        Input message.
 *  @param      PusInputMessageSize     Input message size in bytes.
 *  @param      PrgbKey                 Message authentication key.
 *  @param      PrgbHMAC                Receives the HMAC.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbInputMessage is NULL or PusInputMessageSize is 0.
 */
_Check_return_
unsigned int
Crypt_HMAC(
    _In_bytecount_(PusInputMessageSize)         const BYTE*     PrgbInputMessage,
    _In_                                        UINT16          PusInputMessageSize,
    _In_opt_bytecount_(TSS_SHA1_DIGEST_SIZE)    const BYTE      PrgbKey[TSS_SHA1_DIGEST_SIZE],
    _Out_bytecap_(TSS_SHA1_DIGEST_SIZE)         BYTE            PrgbHMAC[TSS_SHA1_DIGEST_SIZE])
{
    unsigned int unReturnValue = RC_E_FAIL;
    size_t unHmacLength = TSS_SHA1_DIGEST_SIZE;
    EVP_PKEY *pkey = NULL;
    EVP_MD_CTX *mdctx = NULL;

    do
    {
	    // Check parameters
	    if (NULL == PrgbInputMessage || 0 == PusInputMessageSize)
	    {
	        unReturnValue = RC_E_BAD_PARAMETER;
	        break;
	    }

        memset(PrgbHMAC, 0, TSS_SHA1_DIGEST_SIZE);
        mdctx = EVP_MD_CTX_new();
        if(!mdctx)
            break;

        pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, NULL, PrgbKey, TSS_SHA1_DIGEST_SIZE);
        if(!pkey)
            break;

        if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha1(), NULL, pkey))
            break;
        if (1 != EVP_DigestSign(mdctx, PrgbHMAC, &unHmacLength, PrgbInputMessage, PusInputMessageSize))
            break;

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Cleanup
    if (pkey)
        EVP_PKEY_free(pkey);
    if (mdctx)
        EVP_MD_CTX_free(mdctx);

    return unReturnValue;
}

/**
 *  @brief      Calculate hash on the given data
 *  @details    This function calculates a hash on the given data stream (only OpenSSL 1.1.1 or later).
 *
 *  @param      PrgbInputMessage        Input message.
 *  @param      PunInputMessageSize     Input message size in bytes.
 *  @param      PrgbDigest              Receives the calculated digest.
 *  @param      PusDigestSize           Digest size in bytes.
 *  @param      Ptype                   Digest type.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbInputMessage is NULL or PusInputMessageSize is 0.
 */
_Check_return_
unsigned int
Crypt_CalcDigest(
    _In_bytecount_(PusInputMessageSize) const BYTE*     PrgbInputMessage,
    _In_                                const UINT32    PunInputMessageSize,
    _Out_bytecap_(PusDigestSize)        BYTE*           PrgbDigest,
    _In_                                const UINT16    PusDigestSize,
    _In_                                const EVP_MD*   Ptype)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        memset(PrgbDigest, 0, PusDigestSize);

        // Check parameters
        if (NULL == PrgbInputMessage || NULL == PrgbDigest || 0 == PunInputMessageSize || 0 == PusDigestSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        memset(PrgbDigest, 0, PusDigestSize);

	    unsigned int unSize = PusDigestSize;
	    if (1 != EVP_Digest(PrgbInputMessage, PunInputMessageSize, PrgbDigest, &unSize, Ptype, NULL))
            break;

	    if(PusDigestSize != (UINT16)unSize)
	        break;

       unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Calculate SHA-1 on the given data
 *  @details    This function calculates a SHA-1 hash on the given data stream.
 *
 *  @param      PrgbInputMessage        Input message.
 *  @param      PusInputMessageSize     Input message size in bytes.
 *  @param      PrgbSHA1                Receives the SHA-1.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbInputMessage is NULL or PusInputMessageSize is 0.
 */
_Check_return_
unsigned int
Crypt_SHA1(
    _In_bytecount_(PusInputMessageSize) const BYTE*     PrgbInputMessage,
    _In_                                const UINT16    PusInputMessageSize,
    _Out_bytecap_(TSS_SHA1_DIGEST_SIZE) BYTE            PrgbSHA1[TSS_SHA1_DIGEST_SIZE])
{
    return Crypt_CalcDigest(PrgbInputMessage, PusInputMessageSize, PrgbSHA1, TSS_SHA1_DIGEST_SIZE, EVP_sha1());
}

/**
 *  @brief      Calculate SHA-256 on the given data
 *  @details    This function calculates a SHA-256 hash on the given data stream.
 *
 *  @param      PrgbInputMessage        Input message.
 *  @param      PunInputMessageSize     Input message size in bytes.
 *  @param      PrgbSHA256              Receives the SHA-256.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbInputMessage is NULL or PunInputMessageSize is 0.
 */
_Check_return_
unsigned int
Crypt_SHA256(
    _In_bytecount_(PunInputMessageSize)     const BYTE*     PrgbInputMessage,
    _In_                                    const UINT32    PunInputMessageSize,
    _Out_bytecap_(TSS_SHA256_DIGEST_SIZE)   BYTE            PrgbSHA256[TSS_SHA256_DIGEST_SIZE])
{
    return Crypt_CalcDigest(PrgbInputMessage, PunInputMessageSize, PrgbSHA256, TSS_SHA256_DIGEST_SIZE, EVP_sha256());
}

/**
 *  @brief      Calculate SHA-384 on the given data
 *  @details    This function calculates a SHA-384 hash on the given data stream.
 *
 *  @param      PrgbInputMessage        Input message.
 *  @param      PunInputMessageSize     Input message size in bytes.
 *  @param      PrgbSHA384              Receives the SHA-384.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbInputMessage is NULL or PunInputMessageSize is 0.
 */
_Check_return_
unsigned int
Crypt_SHA384(
    _In_bytecount_(PunInputMessageSize)     const BYTE*     PrgbInputMessage,
    _In_                                    const UINT32    PunInputMessageSize,
    _Out_bytecap_(TSS_SHA384_DIGEST_SIZE)   BYTE            PrgbSHA384[TSS_SHA384_DIGEST_SIZE])
{
    return Crypt_CalcDigest(PrgbInputMessage, PunInputMessageSize, PrgbSHA384, TSS_SHA384_DIGEST_SIZE, EVP_sha384());
}

/**
 *  @brief      Calculate SHA-512 on the given data
 *  @details    This function calculates a SHA-512 hash on the given data stream.
 *
 *  @param      PrgbInputMessage        Input message.
 *  @param      PunInputMessageSize     Input message size in bytes.
 *  @param      PrgbSHA512              Receives the SHA-512.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbInputMessage is NULL or PunInputMessageSize is 0.
 */
_Check_return_
unsigned int
Crypt_SHA512(
    _In_bytecount_(PunInputMessageSize)     const BYTE*     PrgbInputMessage,
    _In_                                    const UINT32    PunInputMessageSize,
    _Out_bytecap_(TSS_SHA512_DIGEST_SIZE)   BYTE            PrgbSHA512[TSS_SHA512_DIGEST_SIZE])
{
    return Crypt_CalcDigest(PrgbInputMessage, PunInputMessageSize, PrgbSHA512, TSS_SHA512_DIGEST_SIZE, EVP_sha512());
}

/**
 *  @brief      Seed the pseudo random number generator
 *  @details    This function seeds the pseudo random number generator.
 *
 *  @param      PrgbSeed                Seed.
 *  @param      PusSeedSize             Seed size in bytes.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbSeed is NULL and PusSeedSize is not 0.
 */
_Check_return_
unsigned int
Crypt_SeedRandom(
    _In_bytecount_(PusSeedSize) const BYTE*     PrgbSeed,
    _In_                        const UINT16    PusSeedSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        if (NULL == PrgbSeed && 0 != PusSeedSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        if (PrgbSeed != NULL)
        {
            RAND_seed(PrgbSeed, (UINT32)PusSeedSize);
        }
        else
        {
            // Create default seed from current time
            time_t tSeed = time(NULL);
            RAND_seed(& tSeed, sizeof(tSeed));
        }

        if (1 != RAND_status())
        {
            unReturnValue = RC_E_FAIL;
            break;
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;
    return unReturnValue;
}

/**
 *  @brief      Get random bytes from the pseudo random number generator
 *  @details    This function gets random bytes from the pseudo random number generator.
 *
 *  @param      PusRandomSize           Number of bytes requested.
 *  @param      PrgbRandom              Receives pseudo random bytes.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PrgbRandom is NULL or PusRandomSize is 0.
 */
_Check_return_
unsigned int
Crypt_GetRandom(
    _In_                            const UINT16    PusRandomSize,
    _Out_bytecap_(PusRandomSize)    BYTE*           PrgbRandom)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check input parameters
        if (NULL == PrgbRandom || 0 == PusRandomSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Initialize output data
        memset(PrgbRandom, 0, PusRandomSize);

        // Generate random data
        if (1 != RAND_bytes(PrgbRandom, (UINT32)PusRandomSize))
        {
            unReturnValue = RC_E_FAIL;
            break;
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;
    return unReturnValue;
}

/**
 *  @brief      Create public key from the given input data
 *  @details    The function creates a public key from the given input data (only OpenSSL 3.0 or later).
 *
 *  @param      PunPublicModulusSize            Size of public modulus in bytes.
 *  @param      PrgbPublicModulus               Public modulus buffer.
 *  @param      PunPublicExponentSize           Size of public exponent in bytes.
 *  @param      PrgbPublicExponent              Public exponent buffer.
 *  @param      PppRSAPubKey                    Created pubkey object.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. It was NULL or empty.
 */
_Check_return_
unsigned int
Crypt_GetRSAPubKey(
    _In_                                        UINT32              PunPublicModulusSize,
    _In_bytecount_(PunPublicModulusSize)        const BYTE*         PrgbPublicModulus,
    _In_                                        UINT32              PunPublicExponentSize,
    _In_bytecount_(PunPublicExponentSize)       const BYTE*         PrgbPublicExponent,
    _Out_                                       EVP_PKEY**          PppRSAPubKey)
{
#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_3_0

    unsigned int unReturnValue = RC_E_FAIL;
    BIGNUM* pbnExponent = NULL;
    BIGNUM* pbnPublicModulus = NULL;
    EVP_PKEY_CTX* ctx = NULL;

    OSSL_PARAM_BLD *ossl_params_build = NULL;
    OSSL_PARAM *ossl_params = NULL;

    do
    {
        // Check input parameters
        if (NULL == PppRSAPubKey ||
                NULL == PrgbPublicModulus || 0 == PunPublicModulusSize ||
                NULL == PrgbPublicExponent || 0 == PunPublicExponentSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        *PppRSAPubKey = NULL;

        if (NULL == (ossl_params_build = OSSL_PARAM_BLD_new()))
            break;
        if (NULL == (pbnPublicModulus = BN_bin2bn((const BYTE*)PrgbPublicModulus, PunPublicModulusSize, pbnPublicModulus)))
            break;
        if (1 != OSSL_PARAM_BLD_push_BN(ossl_params_build, "n", pbnPublicModulus))
            break;
        if (NULL == (pbnExponent = BN_bin2bn((const BYTE*)PrgbPublicExponent, PunPublicExponentSize, pbnExponent)))
            break;
        if (1 != OSSL_PARAM_BLD_push_BN(ossl_params_build, "e", pbnExponent))
            break;
        if (NULL == (ossl_params = OSSL_PARAM_BLD_to_param(ossl_params_build)))
            break;
        if (NULL == (ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL)))
            break;
        if (1 != EVP_PKEY_fromdata_init(ctx))
            break;
        if (1 != EVP_PKEY_fromdata(ctx, PppRSAPubKey, EVP_PKEY_PUBLIC_KEY, ossl_params))
            break;

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (NULL != ossl_params_build)
        OSSL_PARAM_BLD_free(ossl_params_build);
    if (NULL != ossl_params)
        OSSL_PARAM_free(ossl_params);
    if (NULL != ctx)
        EVP_PKEY_CTX_free(ctx);

    return unReturnValue;
#else

    unsigned int unReturnValue = RC_E_FAIL;
    BIGNUM* pbnExponent = NULL;
    BIGNUM* pbnPublicModulus = NULL;
    EVP_PKEY* pKey = NULL;
    RSA *pRSAPubKey = NULL;

    do
    {
        // Check input parameters
        if (NULL == PppRSAPubKey ||
                NULL == PrgbPublicModulus || 0 == PunPublicModulusSize ||
                NULL == PrgbPublicExponent || 0 == PunPublicExponentSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        *PppRSAPubKey = NULL;

        // Create RSA key object and set modulus and exponent
        if (NULL == (pRSAPubKey = RSA_new()))
            break;
        if (NULL == (pbnPublicModulus = BN_bin2bn((const BYTE*)PrgbPublicModulus, PunPublicModulusSize, pbnPublicModulus)))
            break;
        if (NULL == (pbnExponent = BN_bin2bn((const BYTE*)PrgbPublicExponent, PunPublicExponentSize, pbnExponent)))
            break;
        if (1 != RSA_set0_key(pRSAPubKey, pbnPublicModulus, pbnExponent, NULL))
            break;
        // Create EVP_PKEY object and set RSA key
        if (NULL == (pKey = EVP_PKEY_new()))
            break;
        if (1 != EVP_PKEY_set1_RSA(pKey, pRSAPubKey))
            break;

        // Return key if successful
        *PppRSAPubKey = pKey;
        pKey = NULL;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Cleanup
    if (NULL != pRSAPubKey)
        RSA_free(pRSAPubKey);
    if (NULL != pKey)
        EVP_PKEY_free(pKey);

    return unReturnValue;

#endif
}

/**
 *  @brief      Encrypt a byte array with a RSA 2048-bit public key
 *  @details    This function encrypts the given data stream with RSA 2048-bit.
 *
 *  @param      PusEncryptionScheme             Encryption scheme. Only CRYPT_ES_RSAESOAEP_SHA1_MGF1 is supported.
 *  @param      PunInputDataSize                Size of input data in bytes.
 *  @param      PrgbInputData                   Input data buffer.
 *  @param      PunPublicModulusSize            Size of public modulus in bytes.
 *  @param      PrgbPublicModulus               Public modulus buffer.
 *  @param      PunPublicExponentSize           Size of public exponent in bytes.
 *  @param      PrgbPublicExponent              Public exponent buffer.
 *  @param      PunLabelSize                    Size of label in bytes.
 *  @param      PrgbLabel                       Label buffer.
 *  @param      PpunEncryptedDataSize           In: Size of buffer for encrypted data in bytes
 *                                              Out: Size of encrypted data in bytes
 *  @param      PrgbEncryptedData               Encrypted data buffer.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred during RSA functionality.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was NULL or empty.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of PrgbEncryptedData is too small.
 *  @retval     RC_E_INTERNAL           In case of a not supported padding schema.
 */
_Check_return_
unsigned int
Crypt_EncryptRSA(
    _In_                                        CRYPT_ENC_SCHEME    PusEncryptionScheme,
    _In_                                        UINT32              PunInputDataSize,
    _In_bytecount_(PunInputDataSize)            const BYTE*         PrgbInputData,
    _In_                                        UINT32              PunPublicModulusSize,
    _In_bytecount_(PunPublicModulusSize)        const BYTE*         PrgbPublicModulus,
    _In_                                        UINT32              PunPublicExponentSize,
    _In_bytecount_(PunPublicExponentSize)       const BYTE*         PrgbPublicExponent,
    _In_                                        UINT32              PunLabelSize,
    _In_bytecount_(PunLabelSize)                const BYTE*         PrgbLabel,
    _Inout_                                     UINT32*             PpunEncryptedDataSize,
    _Inout_bytecap_(*PpunEncryptedDataSize)     BYTE*               PrgbEncryptedData)
{
    UINT32 unReturnValue = RC_E_FAIL;
    size_t encryptedDataSize = 0;
    EVP_PKEY *pRSAPubKey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    BYTE *pLabel = NULL;

    do
    {
        BYTE rgbPaddedBuffer[TSS_MAX_RSA_KEY_BYTES] = {0};
        UINT32 unMaxPaddedBufferSize = RG_LEN(rgbPaddedBuffer);

        // Check input parameters
        if (NULL == PrgbInputData || 0 == PunInputDataSize ||
                NULL == PrgbPublicModulus || 0 == PunPublicModulusSize ||
                NULL == PrgbPublicExponent || 0 == PunPublicExponentSize ||
                NULL == PrgbLabel || 0 == PunLabelSize ||
                NULL == PrgbEncryptedData || NULL == PpunEncryptedDataSize ||
                *PpunEncryptedDataSize > unMaxPaddedBufferSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Check output buffer size (must be at least key length)
        if (*PpunEncryptedDataSize < PunPublicModulusSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            break;
        }

        // Code only supports OAEP padding with SHA1 MGF1
        if (CRYPT_ES_RSAESOAEP_SHA1_MGF1 != PusEncryptionScheme)
        {
            unReturnValue = RC_E_INTERNAL;
            break;
        }

        // Initialize RSA Public Key object
        if (RC_SUCCESS != Crypt_GetRSAPubKey(PunPublicModulusSize, PrgbPublicModulus, PunPublicExponentSize, PrgbPublicExponent, &pRSAPubKey))
            break;

        // Init context and set encryption parameters
        if (NULL == (ctx = EVP_PKEY_CTX_new(pRSAPubKey, NULL)))
            break;
        if (1 != EVP_PKEY_encrypt_init(ctx))
            break;
        if (1 != EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING))
            break;
        if (1 != EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha1()))
            break;
        // Create OPENSSL-specific label buffer for function EVP_PKEY_CTX_set0_rsa_oaep_label()
        if (NULL == (pLabel = OPENSSL_malloc(PunLabelSize)))
            break;
        if (!memcpy(pLabel, PrgbLabel, PunLabelSize))
            break;
        if (1 != EVP_PKEY_CTX_set0_rsa_oaep_label(ctx, pLabel, PunLabelSize))
            break;

        // Encrypt data with public key.
        encryptedDataSize = *PpunEncryptedDataSize;
        if (1 != EVP_PKEY_encrypt(ctx, PrgbEncryptedData, &encryptedDataSize, PrgbInputData, PunInputDataSize))
            break;

        *PpunEncryptedDataSize = (UINT32)encryptedDataSize;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Cleanup
    if (NULL != pRSAPubKey)
        EVP_PKEY_free(pRSAPubKey);
    if (NULL != ctx)
        EVP_PKEY_CTX_free(ctx);

    return unReturnValue;
}

/**
 *  @brief      Verify the given RSA PKCS#1 RSASSA-PSS signature using SHA-256
 *  @details    This function verifies the given RSA PKCS#1 RSASSA-PSS signature using SHA-256 for a RSA 2048-bit public key.
 *
 *  @param      PrgbMessageHash         Message hash buffer.
 *  @param      PunMessageHashSize      Size of message hash buffer.
 *  @param      PrgbSignature           Signature buffer.
 *  @param      PunSignatureSize        Size of the signature buffer.
 *  @param      PrgbModulus             Public modulus buffer.
 *  @param      PunModulusSize          Size of public modulus buffer.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred during RSA functionality.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. An input parameter is NULL or empty.
 *  @retval     RC_E_VERIFY_SIGNATURE   In case the signature is invalid.
 */
_Check_return_
unsigned int
Crypt_VerifySignature(
    _In_bytecount_(PunMessageHashSize)  const BYTE*     PrgbMessageHash,
    _In_                                const UINT32    PunMessageHashSize,
    _In_bytecount_(PunSignatureSize)    const BYTE*     PrgbSignature,
    _In_                                const UINT32    PunSignatureSize,
    _In_bytecount_(PunModulusSize)      const BYTE*     PrgbModulus,
    _In_                                const UINT32    PunModulusSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    EVP_PKEY* pRSAPubKey = NULL;
    EVP_PKEY_CTX* ctx = NULL;

    do
    {
        // Check input parameters
        if (NULL == PrgbMessageHash || 0 == PunMessageHashSize ||
                NULL == PrgbSignature || 0 == PunSignatureSize ||
                NULL == PrgbModulus || RSA2048_MODULUS_SIZE != PunModulusSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Initialize RSA Public Key object
        if (RC_SUCCESS != Crypt_GetRSAPubKey(PunModulusSize, PrgbModulus, sizeof(RSA_DEFAULT_PUB_EXPONENT), RSA_DEFAULT_PUB_EXPONENT, &pRSAPubKey))
            break;

        // Init context and set signature verification parameters
        if (NULL == (ctx = EVP_PKEY_CTX_new(pRSAPubKey, NULL)))
            break;
        if (1 != EVP_PKEY_verify_init(ctx))
            break;
        if (1 != EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PSS_PADDING))
            break;
        if (1 != EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()))
            break;

        // Verify the signature
        if (1 != EVP_PKEY_verify(ctx, PrgbSignature, PunSignatureSize, PrgbMessageHash, PunMessageHashSize))
        {
            unReturnValue = RC_E_VERIFY_SIGNATURE;
            break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Cleanup
    if (NULL != pRSAPubKey)
        EVP_PKEY_free(pRSAPubKey);
    if (NULL != ctx)
        EVP_PKEY_CTX_free(ctx);

    return unReturnValue;
}

/**
 *  @brief      Calculate the CRC value of the given data stream
 *  @details    The function calculates the CRC32 value of a data stream
 *
 *  @param      PpInputData         Data stream for CRC calculation.
 *  @param      PnInputDataSize     Size if data to calculate the CRC.
 *  @param      PpunCRC             Calculated CRC value.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_FAIL           An unexpected error occurred during CRC calculation.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. It was NULL or empty.
 */
_Check_return_
unsigned int
Crypt_CRC(
    _In_bytecount_(PnInputDataSize) const void*     PpInputData,
    _In_                            int             PnInputDataSize,
    _Inout_                         unsigned int*   PpunCRC)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unCRC = 0;
        const unsigned char* pbInputData = (const unsigned char*)PpInputData;

        // Check parameter
        if (NULL == PpunCRC ||
                NULL == PpInputData ||
                0 == PnInputDataSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Calculate CRC value
        unCRC = ~(*PpunCRC);
        while (PnInputDataSize-- != 0)
        {
            int nIndex = 0;
            unCRC ^= *pbInputData++;
            for (; nIndex < 8; nIndex++)
            {
                unCRC = (unCRC >> 1) ^ (-((int)(unCRC & 1)) & CRC32MASKREV);
            }
        }

        *PpunCRC = ~unCRC;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
