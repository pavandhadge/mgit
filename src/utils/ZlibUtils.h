#ifndef ZLIBUTILS_H
#define ZLIBUTILS_H

#include <string>
#include <vector>

std::string decompressZlib(const std::vector<char>& compressed);
std::string compressZlib(const std::string& input);
#endif
