/**
 *	@brief		Implements property storage methods
 *	@details	This module provides a storage for all properties collected during the
 *				program execution.
 *	@file		PropertyStorage.c
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

#include "PropertyStorage.h"
#include "Utility.h"

/// Pointer to the first element in the list
IfxPropertyElement* s_pvHead = NULL;

/// Pointer to the last element in the list
IfxPropertyElement* s_pvTail = NULL;

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
	_In_z_	const wchar_t*	PwszValue)
{
	BOOL fReturnValue = FALSE;
	IfxPropertyElement* pElement = NULL;

	do
	{
		unsigned int unLength = 0;

		// Check parameters
		if (NULL == PwszKey ||
				NULL == PwszValue)
			break;

		// Check length of key and value
		if (RC_E_BUFFER_TOO_SMALL == Platform_StringGetLength(PwszKey, PROPERTY_STORAGE_MAX_KEY, &unLength) ||
				RC_E_BUFFER_TOO_SMALL == Platform_StringGetLength(PwszValue, PROPERTY_STORAGE_MAX_VALUE, &unLength))
			break;

		// Abort if element with same key already exists
		if (PropertyStorage_ExistsElement(PwszKey))
			break;

		// Allocate memory for one property element
		pElement = (IfxPropertyElement*)Platform_MemoryAllocateZero(sizeof(IfxPropertyElement));
		if (NULL == pElement)
			break;

		// Copy key to element
		unLength = PROPERTY_STORAGE_MAX_KEY;
		if (RC_SUCCESS != Platform_StringCopy(pElement->wszKey, &unLength, PwszKey))
			break;

		// Copy value to element
		unLength = PROPERTY_STORAGE_MAX_VALUE;
		if (RC_SUCCESS != Platform_StringCopy(pElement->wszValue, &unLength, PwszValue))
			break;

		// If list is empty, add it to the list
		if (NULL == s_pvHead)
		{
			s_pvHead = pElement;
			s_pvTail = pElement;
		}
		else
		{
			// Update link of the last element in the list
			s_pvTail->pvNextElement = pElement;

			// Update links in the element to add
			pElement->pvPreviousElement = s_pvTail;

			// Update last list element link
			s_pvTail = pElement;
		}

		fReturnValue = TRUE;
	}
	WHILE_FALSE_END;

	// Cleanup in case of error
	if (!fReturnValue)
		Platform_MemoryFree((void**)&pElement);

	return fReturnValue;
}

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
	_In_	BOOL			PfValue)
{
	return PropertyStorage_AddKeyValuePair(PwszKey, PfValue ? L"TRUE" : L"FALSE");
}

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
	_In_z_	const wchar_t*		PwszKey,
	_In_	unsigned int		PunValue)
{
	BOOL fReturnValue = FALSE;
	wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
	unsigned int unValueSize = RG_LEN(wszValue);

	if (RC_SUCCESS == Utility_UInteger2String(PunValue, wszValue, &unValueSize))
		fReturnValue = PropertyStorage_AddKeyValuePair(PwszKey, wszValue);

	return fReturnValue;
}

/**
 *	@brief		Get an element identified by a key
 *	@details	Local helper method to get the element with the given key.
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be changed\n
 *								null-terminated wide char array; max length PROPERTY_STORAGE_MAX_KEY
 *
 *	@returns	The element if found, NULL otherwise
 */
_Check_return_
IfxPropertyElement*
PropertyStorage_GetElementByKey(
	_In_z_ const wchar_t* PwszKey)
{
	IfxPropertyElement* pReturnElement = NULL;

	// Check parameter
	if (NULL != PwszKey)
	{
		// Search list for element
		IfxPropertyElement* pIteratorElement = s_pvHead;
		while (NULL != pIteratorElement)
		{
			// Check if current element contains the given key
			if (0 == Platform_StringCompare(PwszKey, pIteratorElement->wszKey, PROPERTY_STORAGE_MAX_KEY, FALSE))
			{
				// If yes, return it
				pReturnElement = pIteratorElement;
				break;
			}

			// Update iterator
			pIteratorElement = pIteratorElement->pvNextElement;
		}
	}

	return pReturnElement;
}

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
	_In_z_	const wchar_t*	PwszValue)
{
	BOOL fReturnValue = FALSE;

	do
	{
		IfxPropertyElement* pElement = NULL;

		// Check Parameter
		if (NULL == PwszKey ||
				NULL == PwszValue)
			break;

		// Get element to be changed, if existing
		pElement = PropertyStorage_GetElementByKey(PwszKey);
		if (NULL == pElement)
			break;

		// Change value
		{
			unsigned int unValueSize = PROPERTY_STORAGE_MAX_VALUE;
			if (RC_SUCCESS == Platform_StringCopy(pElement->wszValue, &unValueSize, PwszValue))
				fReturnValue = TRUE;
		}
	}
	WHILE_FALSE_END;

	return fReturnValue;
}

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
	_In_	BOOL			PfValue)
{
	return PropertyStorage_ChangeValueByKey(PwszKey, PfValue ? L"TRUE" : L"FALSE");
}

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
	_In_z_	const wchar_t*		PwszKey,
	_In_	unsigned int		PunValue)
{
	BOOL fReturnValue = FALSE;
	wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
	unsigned int unValueSize = RG_LEN(wszValue);

	if (RC_SUCCESS == Utility_UInteger2String(PunValue, wszValue, &unValueSize))
		fReturnValue = PropertyStorage_ChangeValueByKey(PwszKey, wszValue);

	return fReturnValue;
}

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
	_Inout_						unsigned int*	PpunValueSize)
{
	BOOL fReturnValue = FALSE;

	do
	{
		IfxPropertyElement* pElement = NULL;

		// Check parameters
		if (NULL == PwszKey ||
				NULL == PwszValue ||
				NULL == PpunValueSize ||
				0 == *PpunValueSize)
			break;

		// Get element to be read from, if existing
		pElement = PropertyStorage_GetElementByKey(PwszKey);
		if (NULL == pElement)
			break;

		// Get value
		if (RC_SUCCESS == Platform_StringCopy(PwszValue, PpunValueSize, pElement->wszValue))
			fReturnValue = TRUE;
	}
	WHILE_FALSE_END;

	if (FALSE == fReturnValue)
	{
		// Reset out parameters
		if (NULL != PwszValue)
			PwszValue[0] = L'\0';
		if (NULL != PpunValueSize)
			*PpunValueSize = 0;
	}

	return fReturnValue;
}

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
	_Out_	BOOL*			PpfValue)
{
	BOOL fReturnValue = FALSE;

	do
	{
		wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
		unsigned int unValueSize = RG_LEN(wszValue);

		// Check parameters
		if (NULL == PwszKey ||
				NULL == PpfValue)
			break;

		*PpfValue = FALSE;

		if (FALSE == PropertyStorage_GetValueByKey(PwszKey, wszValue, &unValueSize))
			break;

		// Increment size by one due to null-termination
		if (PROPERTY_STORAGE_MAX_VALUE > unValueSize)
			unValueSize++;

		// Convert value to BOOL, if possible
		if (0 == Platform_StringCompare(L"TRUE", wszValue, unValueSize, TRUE))
		{
			*PpfValue = TRUE;
			fReturnValue = TRUE;
		}
		else if (0 == Platform_StringCompare(L"FALSE", wszValue, unValueSize, TRUE))
		{
			*PpfValue = FALSE;
			fReturnValue = TRUE;
		}
	}
	WHILE_FALSE_END;

	return fReturnValue;
}

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
	_Out_	unsigned int*		PpunValue)
{
	BOOL fReturnValue = FALSE;

	do
	{
		wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
		unsigned int unValueSize = RG_LEN(wszValue);
		int nIndex = -1;

		// Check parameters
		if (NULL == PwszKey ||
				NULL == PpunValue)
			break;

		if (FALSE == PropertyStorage_GetValueByKey(PwszKey, wszValue, &unValueSize))
			break;

		// Increment size by one due to null-termination
		if (PROPERTY_STORAGE_MAX_VALUE > unValueSize)
			unValueSize++;

		// Convert String to Integer
		if (RC_SUCCESS == Utility_StringContainsWChar(wszValue, unValueSize, L'x', &nIndex))
		{
			if (nIndex == -1)
			{
				// Convert as decimal value
				if (RC_SUCCESS == Utility_StringParseUInteger(wszValue, unValueSize, PpunValue))
					fReturnValue = TRUE;
			}
			else
			{
				// Convert as hex value
				if (RC_SUCCESS == Utility_UIntegerParseHexString(wszValue, PpunValue))
					fReturnValue = TRUE;
			}
		}
	}
	WHILE_FALSE_END;

	if (FALSE == fReturnValue)
		// Reset out parameters
		if (NULL != PpunValue)
			*PpunValue = 0;

	return fReturnValue;
}

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
	_In_z_ const wchar_t* PwszKey)
{
	// Try to get the element with the given key
	return NULL != PropertyStorage_GetElementByKey(PwszKey);
}

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
	_In_z_ const wchar_t* PwszKey)
{
	BOOL fReturnValue = FALSE;

	do
	{
		IfxPropertyElement* pElement = NULL;

		// Check parameter
		if (NULL == PwszKey)
			break;

		// Get element to be removed, if existing
		pElement = PropertyStorage_GetElementByKey(PwszKey);
		if (NULL != pElement)
		{
			// Get previous and next elements
			IfxPropertyElement* pPrev = pElement->pvPreviousElement;
			IfxPropertyElement* pNext = pElement->pvNextElement;

			// Hang out the element now
			if (NULL != pPrev)
				pPrev->pvNextElement = pNext;
			if (NULL != pNext)
				pNext->pvPreviousElement = pPrev;

			// Check if element to be removed is the first or last element
			if (s_pvHead == pElement)
				s_pvHead = pNext;
			if (s_pvTail == pElement)
				s_pvTail = pPrev;

			// Free memory of element to be removed
			Platform_MemoryFree((void**)&pElement);

			fReturnValue = TRUE;
		}
	}
	WHILE_FALSE_END;

	return fReturnValue;
}

/**
 *	@brief		Clears the whole property list
 *	@details
 */
void
PropertyStorage_ClearElements()
{
	// Start with the first element
	IfxPropertyElement* pIteratorElement = s_pvHead;

	// Walk along until the end of the list has been reached
	while (NULL != pIteratorElement)
	{
		// Remember pointer of current element
		IfxPropertyElement* pElement = pIteratorElement;

		// Update iterator
		pIteratorElement = pIteratorElement->pvNextElement;

		// Free memory of current element
		Platform_MemoryFree((void**)&pElement);
	}

	// Clear pointers to first and last element
	s_pvHead = NULL;
	s_pvTail = NULL;
}

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
	_In_	unsigned long long	PullValue)
{
	BOOL fReturnValue = FALSE;
	wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
	unsigned int unValueSize = RG_LEN(wszValue);

	if (RC_SUCCESS == Utility_ULongLong2String(PullValue, wszValue, &unValueSize))
		fReturnValue = PropertyStorage_AddKeyValuePair(PwszKey, wszValue);

	return fReturnValue;
}

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
	_In_	unsigned long long	PullValue)
{
	BOOL fReturnValue = FALSE;
	wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
	unsigned int unValueSize = RG_LEN(wszValue);

	if (RC_SUCCESS == Utility_ULongLong2String(PullValue, wszValue, &unValueSize))
		fReturnValue = PropertyStorage_ChangeValueByKey(PwszKey, wszValue);

	return fReturnValue;
}

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
	_Out_	unsigned long long*	PpullValue)
{
	BOOL fReturnValue = FALSE;

	do
	{
		wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE] = {0};
		unsigned int unValueSize = RG_LEN(wszValue);

		// Check parameters
		if (NULL == PwszKey || NULL == PpullValue)
			break;

		if (FALSE == PropertyStorage_GetValueByKey(PwszKey, wszValue, &unValueSize))
			break;

		// Increment size by one due to null-termination
		if (PROPERTY_STORAGE_MAX_VALUE > unValueSize)
			unValueSize++;

		// Convert String to Integer
		if (RC_SUCCESS == Utility_StringParseULongLong(wszValue, unValueSize, PpullValue))
			fReturnValue = TRUE;
	}
	WHILE_FALSE_END;

	if (FALSE == fReturnValue)
		// Reset out parameters
		if (NULL != PpullValue)
			*PpullValue = 0;

	return fReturnValue;
}