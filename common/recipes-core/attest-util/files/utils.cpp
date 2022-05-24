#include <utils.hpp>
#include <iostream>
#include <iomanip>
#include "nlohmann/json.hpp"

void handleResponseRaw(string response, string error) {
  std::cout << response << std::endl;
  if (error != "Success")
    std::cerr << error << std::endl;
}

void handleResponseJson(string response, string error) {
  nlohmann::json jsonResponse;

  jsonResponse["version"] = VERSION;
  jsonResponse["status"] = error;
  jsonResponse["payload"] = response;

  std::cout << jsonResponse.dump() << std::endl;
}

static const string characterSet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline void decodeChunk(
    vector<uint8_t>& characters,
    vector<uint8_t>& decodedBytes,
    int end) {
  vector<uint8_t> values(3, 0);

  for (int i = 0; i < (int)characters.size(); ++i)
    characters[i] = characterSet.find(characters[i]);

  values[0] = (characters[0] << 2) + ((characters[1] & 0x30) >> 4);
  values[1] = ((characters[1] & 0xf) << 4) + ((characters[2] & 0x3c) >> 2);
  values[2] = characters[2] = ((characters[2] & 0x3) << 6) + characters[3];

  for (int i = 0; i < end; i++) {
    decodedBytes.push_back(values[i]);
  }
}

vector<uint8_t> decodeBase64(string& encodedString) {
  vector<uint8_t> characters(4, 0);
  vector<uint8_t> decodedBytes;
  int i = 0;

  for (char nextChar : encodedString) {
    if (nextChar == '=')
      break;

    characters[i] = nextChar;

    if (i == 3) {
      decodeChunk(characters, decodedBytes, i);
      i = 0;
    } else {
      ++i;
    }
  }

  if (i) {
    for (int j = i; j < 4; j++)
      characters[j] = 0;

    decodeChunk(characters, decodedBytes, i - 1);
  }

  return decodedBytes;
}

static inline string accumulateBytes(const vector<uint8_t>& temp, int end) {
  vector<uint8_t> accumulator(4, 0);
  string encoded;

  accumulator[0] = (temp[0] & 0xfc) >> 2;
  accumulator[1] = ((temp[0] & 0x03) << 4) + ((temp[1] & 0xf0) >> 4);
  accumulator[2] = ((temp[1] & 0x0f) << 2) + ((temp[2] & 0xc0) >> 6);
  accumulator[3] = temp[2] & 0x3f;

  for (int i = 0; i < end; ++i)
    encoded += characterSet[accumulator[i]];

  return encoded;
}

string encodeBase64(vector<uint8_t>& bytes) {
  string encodedBytes;
  int i = 0;
  vector<uint8_t> temp(3, 0);

  // Handle 24 bits at a time
  for (uint8_t nextByte : bytes) {
    temp[i++] = nextByte;

    if (i == 3) {
      encodedBytes += accumulateBytes(temp, 4);
      i = 0;
    }
  }

  // Handle the case that the number of input bits is not divisible by 24
  if (i != 0) {
    for (int j = i; j < (int)temp.size(); j++)
      temp[j] = '\0';

    encodedBytes += accumulateBytes(temp, i + 1);

    for (; i < (int)temp.size(); ++i)
      encodedBytes += '=';
  }

  return encodedBytes;
}


void printHexValues(uint8_t *values, size_t size) {
  for(size_t index = 0; index < size; ++index) {
    if(index != 0 && index % 16 == 0)
      std::cout << std::endl;

    std::cout << std::setfill('0')
              << std::setw(2)
              << std::right
              << std::hex
              << +values[index]
              << " ";
  }

  std::cout << std::endl;
}

vector<string> splitMessage(string message, string delimiter) {
    auto end = string::npos;
    vector<string> messages;

    while((end = message.find(delimiter)) != string::npos) {
        messages.push_back(message.substr(0, end));
        message.erase(0, end + delimiter.length());
    }

    // Don't forget last message, assuming they didn't end with a delimiter.
    if(message.length() > 0)
        messages.push_back(message);

    return messages;
}
