#pragma once
#include <string>
#include <vector>

std::string decompressZlib(const std::vector<char>& compressed);
std::string compressZlib(const std::string& input);
std::string hash_sha1(const std::string& data);
std::string getCurrentTimestampWithTimezone();
std::string hexToBinary(const std::string& hex);
std::string binaryToHex(const std::string& binary);
