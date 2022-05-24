#pragma once

#include <stdint.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

const string VERSION = "0.1";

/**
 * This function takes a message response and an error string
 * and outputs the response to stdout and if the error string is not
 * empty it outputs it to stderr.
 */
void handleResponseRaw(string response, string error);

/**
 * This function takes the response and error strings as well as
 * the utility versions and packages them into a json format before
 * outputting them to stdout
 */
void handleResponseJson(string response, string error);

/**
 * This function takes a string encoded as base64 and decodes into an unsigned
 * byte vector using the "a-zA-Z+/" encoding character set. This function
 * assumes an '=' padded encoded string.
 */
vector<uint8_t> decodeBase64(string& encodedString);

/**
 * This function takes a unsigned byte vector and encodes it into a
 * base64 string. It encodes it using the "a-zA-Z+/" character set and
 * uses '=' as padding when the byte count is not divisible by 3.
 */
string encodeBase64(std::vector<uint8_t>& bytes);

/** Prints an array of bytes as 2-character hex values. */
void printHexValues(uint8_t *values, size_t size);


vector<string> splitMessage(string message, string delimiter);
