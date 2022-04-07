/**
 *	@brief		Declares the Cryptography interface
 *	@details	This module provides cryptographic functions.
 *	@file		Crypt.h
 *	@copyright	Copyright 2014 - 2018 Infineon Technologies AG ( www.infineon.com )
 *
 *	@copyright	All rights reserved.
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
#define CRYPT_ES_RSAESOAEP_SHA1_MGF1	0x0003

/// Size of PSS padding salt in bytes
#define CRYPT_PSS_PADDING_SALT_SIZE 32

/// Data type for encryption scheme
typedef UINT16 CRYPT_ENC_SCHEME;

/// Public exponent for firmware image signature
static const BYTE RSA_PUB_EXPONENT_KEY_ID_0[]	= { 0x01, 0x00, 0x01 };

/// Public modulus for firmware image signature
static const BYTE RSA_PUB_MODULUS_KEY_ID_0[]	= { 0xc9, 0x70, 0xa4, 0x12, 0x73, 0x0d, 0x84, 0xe8, 0x79, 0x3e, 0x5d, 0x64, 0x8a, 0xb3, 0x3a, 0x77,
													0xb8, 0xd4, 0x18, 0x9c, 0x0b, 0xe4, 0xfb, 0xf5, 0x60, 0x09, 0xe7, 0x5e, 0xc4, 0x65, 0x42, 0xa8,
													0x87, 0x62, 0x3f, 0x9d, 0x20, 0x50, 0x09, 0xcf, 0x0a, 0xbf, 0xfd, 0x14, 0x22, 0x9a, 0xf1, 0x29,
													0x33, 0x70, 0xd9, 0xec, 0xde, 0x94, 0xfb, 0xf9, 0xb9, 0x45, 0xdf, 0x56, 0xea, 0x44, 0x0a, 0xa0,
													0xf5, 0x8c, 0x84, 0x55, 0xfd, 0x2f, 0xa8, 0x40, 0xbd, 0x71, 0xae, 0x94, 0x5d, 0xfc, 0x54, 0xdb,
													0x4f, 0x08, 0x71, 0x08, 0xd7, 0x7f, 0xd6, 0x8c, 0x7e, 0xe0, 0xab, 0x63, 0xfb, 0x7c, 0x4a, 0xab,
													0x6e, 0xdc, 0xd0, 0x7a, 0x41, 0x7b, 0x9c, 0xed, 0x55, 0x8f, 0x81, 0xc4, 0x22, 0x1d, 0xbc, 0x80,
													0x64, 0xa0, 0x10, 0x2f, 0xcb, 0x15, 0xcb, 0x04, 0x35, 0x37, 0x15, 0xc9, 0x7d, 0x7b, 0x13, 0x06,
													0x8b, 0xe1, 0x2a, 0x87, 0x40, 0xb3, 0xe0, 0xf1, 0xe6, 0x65, 0xc8, 0x9a, 0x20, 0x9d, 0x85, 0x74,
													0x0b, 0xd8, 0xc4, 0xd4, 0x1e, 0x04, 0x5e, 0x05, 0x4d, 0x8f, 0xb3, 0xc7, 0x64, 0x46, 0x87, 0x4a,
													0x9d, 0x6e, 0xc9, 0xd1, 0xd4, 0xcf, 0x61, 0xd8, 0x83, 0x2f, 0x80, 0x36, 0xbd, 0x98, 0x11, 0xae,
													0xa5, 0x93, 0x18, 0x99, 0xae, 0x9d, 0x13, 0x77, 0x2d, 0x4c, 0x53, 0xda, 0xb5, 0xe4, 0xdd, 0xdc,
													0xe8, 0x60, 0x1e, 0x73, 0x22, 0x02, 0xe6, 0x4b, 0xa1, 0xce, 0x6f, 0x27, 0x12, 0x2d, 0x3d, 0x98,
													0x27, 0x0a, 0x4b, 0x47, 0x53, 0xa3, 0x92, 0x19, 0x69, 0x7c, 0x29, 0x9e, 0xda, 0x83, 0x69, 0x22,
													0xa8, 0x1c, 0x5f, 0x4b, 0x82, 0xce, 0x98, 0x7b, 0x8e, 0x69, 0x2c, 0xc6, 0x1f, 0xe4, 0xc2, 0x0c,
													0x7a, 0x1f, 0x03, 0xd4, 0x15, 0x10, 0xa8, 0xd6, 0xf9, 0x68, 0xc1, 0x75, 0x29, 0xa8, 0x85, 0xb1
											   };

/**
 *	@brief		Calculate HMAC-SHA-1 on the given message
 *	@details	This function calculates a HMAC-SHA-1 on the input message.
 *
 *	@param		PrgbInputMessage		Input message
 *	@param		PusInputMessageSize		Input message size in bytes
 *	@param		PrgbKey					Message authentication key
 *	@param		PrgbHMAC				Receives the HMAC
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PrgbInputMessage is NULL or PusInputMessageSize is 0
 */
_Check_return_
unsigned int
Crypt_HMAC(
	_In_bytecount_(PusInputMessageSize)			const BYTE*		PrgbInputMessage,
	_In_										UINT16			PusInputMessageSize,
	_In_opt_bytecount_(TSS_SHA1_DIGEST_SIZE)	const BYTE		PrgbKey[TSS_SHA1_DIGEST_SIZE],
	_Out_bytecap_(TSS_SHA1_DIGEST_SIZE)			BYTE			PrgbHMAC[TSS_SHA1_DIGEST_SIZE]);

/**
 *	@brief		Calculate SHA-1 on the given data
 *	@details	This function calculates a SHA-1 hash on the given data stream.
 *
 *	@param		PrgbInputMessage		Input message
 *	@param		PusInputMessageSize		Input message size in bytes
 *	@param		PrgbSHA1				Receives the SHA-1
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PrgbInputMessage is NULL or PusInputMessageSize is 0
 */
_Check_return_
unsigned int
Crypt_SHA1(
	_In_bytecount_(PusInputMessageSize)		const BYTE*		PrgbInputMessage,
	_In_									const UINT16	PusInputMessageSize,
	_Out_bytecap_(TSS_SHA1_DIGEST_SIZE)		BYTE			PrgbSHA1[TSS_SHA1_DIGEST_SIZE]);

/**
 *	@brief		Calculate SHA-256 on the given data
 *	@details	This function calculates a SHA-256 hash on the given data stream.
 *
 *	@param		PrgbInputMessage		Input message
 *	@param		PunInputMessageSize		Input message size in bytes
 *	@param		PrgbSHA256				Receives the SHA-256
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PrgbInputMessage is NULL or PunInputMessageSize is 0
 */
_Check_return_
unsigned int
Crypt_SHA256(
	_In_bytecount_(PunInputMessageSize)		const BYTE*			PrgbInputMessage,
	_In_									const unsigned int	PunInputMessageSize,
	_Out_bytecap_(TSS_SHA256_DIGEST_SIZE)	BYTE				PrgbSHA256[TSS_SHA256_DIGEST_SIZE]);

/**
 *	@brief		Seed the pseudo random number generator
 *	@details	This function seeds the pseudo random number generator.
 *
 *	@param		PrgbSeed				Seed
 *	@param		PusSeedSize				Seed size in bytes
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PrgbSeed is NULL and PusSeedSize is not 0.
 */
_Check_return_
unsigned int
Crypt_SeedRandom(
	_In_bytecount_(PusSeedSize)			const BYTE*		PrgbSeed,
	_In_								const UINT16	PusSeedSize);

/**
 *	@brief		Get random bytes from the pseudo random number generator
 *	@details	This function gets random bytes from the pseudo random number generator.
 *
 *	@param		PusRandomSize			Number of bytes requested.
 *	@param		PrgbRandom				Receives pseudo random bytes.
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PrgbRandom is NULL or PusRandomSize is 0
 */
_Check_return_
unsigned int
Crypt_GetRandom(
	_In_							const UINT16	PusRandomSize,
	_Out_bytecap_(PusRandomSize)	BYTE*			PrgbRandom);

/**
 *	@brief		Encrypt a byte array with a RSA 2048-bit public key
 *	@details	This function encrypts the given data stream with RSA 2048-bit.
 *
 *	@param		PusEncryptionScheme				Encryption scheme. Only CRYPT_ES_RSAESOAEP_SHA1_MGF1 is supported.
 *	@param		PunInputDataSize				Size of input data in bytes
 *	@param		PrgbInputData					Input data buffer
 *	@param		PunPublicModulusSize			Size of public modulus in bytes
 *	@param		PrgbPublicModulus				Public modulus buffer
 *	@param		PunPublicExponentSize			Size of public exponent in bytes
 *	@param		PrgbPublicExponent				Public exponent buffer
 *	@param		PunLabelSize					Size of label in bytes
 *	@param		PrgbLabel						Label buffer
 *	@param		PpunEncryptedDataSize			In: Size of buffer for encrypted data in bytes
 *												Out: Size of encrypted data in bytes
 *	@param		PrgbEncryptedData				Encrypted data buffer
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred during RSA functionality.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was NULL or empty.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of PrgbEncryptedData is too small
 *	@retval		RC_E_INTERNAL			In case of a not supported padding schema
 */
_Check_return_
unsigned int
Crypt_EncryptRSA(
	_In_										CRYPT_ENC_SCHEME	PusEncryptionScheme,
	_In_										unsigned int		PunInputDataSize,
	_In_bytecount_(PunInputDataSize)			const BYTE*			PrgbInputData,
	_In_										unsigned int		PunPublicModulusSize,
	_In_bytecount_(PunPublicModulusSize)		const BYTE*			PrgbPublicModulus,
	_In_										unsigned int		PunPublicExponentSize,
	_In_bytecount_(PunPublicExponentSize)		const BYTE*			PrgbPublicExponent,
	_In_										unsigned int		PunLabelSize,
	_In_bytecount_(PunLabelSize)				const BYTE*			PrgbLabel,
	_Inout_										unsigned int*		PpunEncryptedDataSize,
	_Inout_bytecap_(*PpunEncryptedDataSize)		BYTE*				PrgbEncryptedData);

/**
 *	@brief		Verify the given RSA PKCS#1 RSASSA-PSS signature
 *	@details	This function verifies the given RSA PKCS#1 RSASSA-PSS signature with a RSA 2048-bit public key.
 *
 *	@param		PrgbMessageHash			Message hash buffer
 *	@param		PunMessageHashSize		Size of message hash buffer
 *	@param		PrgbSignature			Signature buffer
 *	@param		PunSignatureSize		Size of the signature buffer
 *	@param		PrgbModulus				Public modulus buffer
 *	@param		PunModulusSize			Size of public modulus buffer
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred during RSA functionality.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. An input parameter is NULL or empty
 *	@retval		RC_E_VERIFY_SIGNATURE	In case the signature is invalid
 */
_Check_return_
unsigned int
Crypt_VerifySignature(
	_In_bytecount_(PunMessageHashSize)	const BYTE*			PrgbMessageHash,
	_In_								const unsigned int	PunMessageHashSize,
	_In_bytecount_(PunSignatureSize)	const BYTE*			PrgbSignature,
	_In_								const unsigned int	PunSignatureSize,
	_In_bytecount_(PunModulusSize)		const BYTE*			PrgbModulus,
	_In_								const unsigned int	PunModulusSize);

/**
 *	@brief		Calculate the CRC value of the given data stream
 *	@details	The function calculates bitwise a CRC over a data stream
 *
 *	@param		PpInputData			Data stream for CRC calculation
 *	@param		PnInputDataSize		Size if data to calculate the CRC
 *	@param		PpunCRC				Calculated CRC value
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_FAIL			An unexpected error occurred during CRC calculation.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. It was NULL or empty.
 */
_Check_return_
unsigned int
Crypt_CRC(
	_In_bytecount_(PnInputDataSize)	const void*		PpInputData,
	_In_							int				PnInputDataSize,
	_Inout_							unsigned int*	PpunCRC);

#ifdef __cplusplus
}
#endif
