/**
 *  @brief      Declares the Cryptography interface
 *  @details    This module provides cryptographic functions.
 *  @file       Crypt.h
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "StdInclude.h"
#include "../MicroTss/Tpm_2_0/TPM2_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Identifier for RSAES_OAEP encryption scheme
#define CRYPT_ES_RSAESOAEP_SHA1_MGF1    0x0003

/// Size of PSS padding salt in bytes
#define CRYPT_PSS_PADDING_SALT_SIZE 32

/// Data type for encryption scheme
typedef UINT16 CRYPT_ENC_SCHEME;

/// Size of the modulus for an RSA2048 key pair.
#define RSA2048_MODULUS_SIZE    256

/// Public exponent for firmware image signature
static const BYTE RSA_DEFAULT_PUB_EXPONENT[]   = { 0x01, 0x00, 0x01 };

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
    _Out_bytecap_(TSS_SHA1_DIGEST_SIZE)         BYTE            PrgbHMAC[TSS_SHA1_DIGEST_SIZE]);

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
    _In_bytecount_(PusInputMessageSize)     const BYTE*     PrgbInputMessage,
    _In_                                    const UINT16    PusInputMessageSize,
    _Out_bytecap_(TSS_SHA1_DIGEST_SIZE)     BYTE            PrgbSHA1[TSS_SHA1_DIGEST_SIZE]);

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
    _In_bytecount_(PunInputMessageSize)     const BYTE*         PrgbInputMessage,
    _In_                                    const unsigned int  PunInputMessageSize,
    _Out_bytecap_(TSS_SHA256_DIGEST_SIZE)   BYTE                PrgbSHA256[TSS_SHA256_DIGEST_SIZE]);

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
    _In_bytecount_(PunInputMessageSize)     const BYTE*         PrgbInputMessage,
    _In_                                    const unsigned int  PunInputMessageSize,
    _Out_bytecap_(TSS_SHA384_DIGEST_SIZE)   BYTE                PrgbSHA384[TSS_SHA384_DIGEST_SIZE]);

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
    _In_bytecount_(PunInputMessageSize)     const BYTE*         PrgbInputMessage,
    _In_                                    const unsigned int  PunInputMessageSize,
    _Out_bytecap_(TSS_SHA512_DIGEST_SIZE)   BYTE                PrgbSHA512[TSS_SHA512_DIGEST_SIZE]);

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
    _In_bytecount_(PusSeedSize)         const BYTE*     PrgbSeed,
    _In_                                const UINT16    PusSeedSize);

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
    _Out_bytecap_(PusRandomSize)    BYTE*           PrgbRandom);

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
    _In_                                        unsigned int        PunInputDataSize,
    _In_bytecount_(PunInputDataSize)            const BYTE*         PrgbInputData,
    _In_                                        unsigned int        PunPublicModulusSize,
    _In_bytecount_(PunPublicModulusSize)        const BYTE*         PrgbPublicModulus,
    _In_                                        unsigned int        PunPublicExponentSize,
    _In_bytecount_(PunPublicExponentSize)       const BYTE*         PrgbPublicExponent,
    _In_                                        unsigned int        PunLabelSize,
    _In_bytecount_(PunLabelSize)                const BYTE*         PrgbLabel,
    _Inout_                                     unsigned int*       PpunEncryptedDataSize,
    _Inout_bytecap_(*PpunEncryptedDataSize)     BYTE*               PrgbEncryptedData);

/**
 *  @brief      Verify the given RSA PKCS#1 RSASSA-PSS signature
 *  @details    This function verifies the given RSA PKCS#1 RSASSA-PSS signature with a RSA 2048-bit public key.
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
    _In_bytecount_(PunMessageHashSize)  const BYTE*         PrgbMessageHash,
    _In_                                const unsigned int  PunMessageHashSize,
    _In_bytecount_(PunSignatureSize)    const BYTE*         PrgbSignature,
    _In_                                const unsigned int  PunSignatureSize,
    _In_bytecount_(PunModulusSize)      const BYTE*         PrgbModulus,
    _In_                                const unsigned int  PunModulusSize);

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
    _Inout_                         unsigned int*   PpunCRC);

#ifdef __cplusplus
}
#endif
