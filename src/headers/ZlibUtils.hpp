#pragma once
#include <string>
#include <vector>

std::string decompressZlib(const std::vector<char>& compressed);
std::string compressZlib(const std::string& input);
std::string getCurrentTimestampWithTimezone();
