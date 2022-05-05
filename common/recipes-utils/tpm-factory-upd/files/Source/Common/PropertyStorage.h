/**
 *	@brief		Declares property storage methods
 *	@details	This module provides a storage for all properties collected during the
 *				program execution.
 *	@file		PropertyStorage.h
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

#ifdef __cplusplus
extern "C" {
#endif

/// Define property storage max key size
#define PROPERTY_STORAGE_MAX_KEY	MAX_NAME + 1
/// Define property storage max value size
#define PROPERTY_STORAGE_MAX_VALUE	MAX_PATH + 1

/**
 *	@brief		This structure is used to store a property in a double linked list
 *	@details	Each element has a unique key and a value.
 */
typedef struct tdIfxPropertyElement
{
	/// Pointer to the previous IfxPropertyElement (NULL if the first element)
	struct tdIfxPropertyElement*	pvPreviousElement;
	/// Pointer to the next IfxPropertyElement (NULL if the last element)
	struct tdIfxPropertyElement*	pvNextElement;
	/// Key to identify the IfxPropertyElement
	wchar_t							wszKey[PROPERTY_STORAGE_MAX_KEY];
	/// Value for the IfxPropertyElement
	wchar_t							wszValue[PROPERTY_STORAGE_MAX_VALUE];
} IfxPropertyElement;

/**
 *	@brief		Add a key value pair to the PropertyStorage
 *	@details	Operation fails in case an element with same key already exists.
 *
 *	@param		PwszKey			Unique key identifier for the PropertyElement to be added\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PwszValue		Pointer to a wide char array containing the value\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_VALUE
 *
 *	@retval		TRUE		If the element has been added
 *	@retval		FALSE		If the element could not be added, e.g. because element with same key already exists
 */
_Check_return_
BOOL
PropertyStorage_AddKeyValuePair(
	_In_z_	const wchar_t*	PwszKey,
	_In_z_	const wchar_t*	PwszValue);

/**
 *	@brief		Add a key and a boolean value pair to the PropertyStorage
 *	@details
 *
 *	@param		PwszKey			Unique key identifier for the PropertyElement to be added\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PfValue			Boolean value to add
 *
 *	@retval		TRUE		If the element has been added
 *	@retval		FALSE		If the element could not be added, e.g. because element with same key already exists
 */
_Check_return_
BOOL
PropertyStorage_AddKeyBooleanValuePair(
	_In_z_	const wchar_t*	PwszKey,
	_In_	BOOL			PfValue);

/**
 *	@brief		Add a key and a unsigned int value pair to the PropertyStorage
 *	@details
 *
 *	@param		PwszKey			Unique key identifier for the PropertyElement to be added\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PunValue		Unsigned integer value to add
 *
 *	@retval		TRUE		If the element has been added
 *	@retval		FALSE		If the element could not be added, e.g. because element with same key already exists
 */
_Check_return_
BOOL
PropertyStorage_AddKeyUIntegerValuePair(
	_In_z_	const wchar_t*	PwszKey,
	_In_	unsigned int	PunValue);

/**
 *	@brief		Change the value of an element identified by a key
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be changed\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PwszValue		Pointer to a wide char array containing the new value\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_VALUE
 *
 *	@retval		TRUE		If the element value has been changed
 *	@retval		FALSE		If the element could not be changed, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_ChangeValueByKey(
	_In_z_	const wchar_t*	PwszKey,
	_In_z_	const wchar_t*	PwszValue);

/**
 *	@brief		Change the boolean value of an element identified by a key
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be changed\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PfValue			Boolean value for changing
 *
 *	@retval		TRUE		If the element value has been changed
 *	@retval		FALSE		If the element could not be changed, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_ChangeBooleanValueByKey(
	_In_z_	const wchar_t*	PwszKey,
	_In_	BOOL			PfValue);

/**
 *	@brief		Change the unsigned integer value of an element identified by a key
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be changed\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PunValue		Unsigned integer value for changing
 *
 *	@retval		TRUE		If the element value has been changed
 *	@retval		FALSE		If the element could not be changed, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_ChangeUIntegerValueByKey(
	_In_z_	const wchar_t*	PwszKey,
	_In_	unsigned int	PunValue);

/**
 *	@brief		Get a copy of the PropertyElement's value with the given key
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to obtain the value from\n
 *								null-terminated wide char array
 *	@param		PwszValue		Pointer to a wide char array containing the PropertyElement value
 *	@param		PpunValueSize	In: the capacity of the wide char array in elements (incl. termination zero)\n
 *								Out: the length of the string copied to (without termination zero)
 *
 *	@retval		TRUE		If the operation was successful
 *	@retval		FALSE		If the value could not be retrieved, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_GetValueByKey(
	_In_z_						const wchar_t*	PwszKey,
	_Out_z_cap_(*PpunValueSize)	wchar_t*		PwszValue,
	_Inout_						unsigned int*	PpunValueSize);

/**
 *	@brief		Get the value of the PropertyElement with the given key casted to a BOOL
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to obtain the value from\n
 *								null-terminated wide char array
 *	@param		PpfValue		Pointer to a BOOL containing the PropertyElement value
 *
 *	@retval		TRUE		If the operation was successful
 *	@retval		FALSE		If the value could not be retrieved, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_GetBooleanValueByKey(
	_In_z_	const wchar_t*	PwszKey,
	_Out_	BOOL*			PpfValue);

/**
 *	@brief		Get the value of the PropertyElement with the given key casted to an unsigned integer
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to obtain the value from\n
 *								null-terminated wide char array
 *	@param		PpunValue		Pointer to an unsigned integer containing the PropertyElement value
 *
 *	@retval		TRUE		If the operation was successful
 *	@retval		FALSE		If the value could not be retrieved, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_GetUIntegerValueByKey(
	_In_z_	const wchar_t*		PwszKey,
	_Out_	unsigned int*		PpunValue);

/**
 *	@brief		Checks the existence of an element with a given key
 *	@details
 *
 *	@param		PwszKey		Key identifier for the PropertyElement to be searched for
 *
 *	@retval		TRUE		If an element was found with the given key
 *	@retval		FALSE		If no element was found with the given key
 */
_Check_return_
BOOL
PropertyStorage_ExistsElement(
	_In_z_ const wchar_t* PwszKey);

/**
 *	@brief		Removes the PropertyElement identified by a given key
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be removed\n
 *								null-terminated wide char array
 *
 *	@retval		TRUE		If the element has been removed
 *	@retval		FALSE		If the element could not be removed, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_RemoveElement(
	_In_z_ const wchar_t* PwszKey);

/**
 *	@brief		Clears the whole property list
 *	@details
 */
void
PropertyStorage_ClearElements();

/**
 *	@brief		Add a key and a unsigned long long value pair to the PropertyStorage
 *	@details
 *
 *	@param		PwszKey			Unique key identifier for the PropertyElement to be added\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PullValue		Unsigned long long (64bit) value to add
 *
 *	@retval		TRUE		If the element has been added
 *	@retval		FALSE		If the element could not be added, e.g. because element with same key already exists
 */
_Check_return_
BOOL
PropertyStorage_AddKeyULongLongValuePair(
	_In_z_	const wchar_t*		PwszKey,
	_In_	unsigned long long	PullValue);

/**
 *	@brief		Change the unsigned long long value of an element identified by a key
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be changed\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *	@param		PullValue		Unsigned long long value for changing
 *
 *	@retval		TRUE		If the element value has been changed
 *	@retval		FALSE		If the element could not be changed, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_ChangeULongLongValueByKey(
	_In_z_	const wchar_t*		PwszKey,
	_In_	unsigned long long	PullValue);

/**
 *	@brief		Get the value of the PropertyElement with the given key casted to an unsigned long long
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to obtain the value from\n
 *								null-terminated wide char array
 *	@param		PpullValue		Pointer to an unsigned long long containing the PropertyElement value
 *
 *	@retval		TRUE		If the operation was successful
 *	@retval		FALSE		If the value could not be retrieved, e.g. because no element with the given key has been found
 */
_Check_return_
BOOL
PropertyStorage_GetULongLongValueByKey(
	_In_z_	const wchar_t*		PwszKey,
	_Out_	unsigned long long*	PpullValue);

#ifdef __cplusplus
}
#endif
