#include <vector>
#include <string>
#include <stdexcept>
#include "./headers/util.h"
#include <zlib.h>
#include <iostream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

std::string decompressZlib(const std::vector<char>& compressed) {
    std::vector<char> output(100000); // Start with 100KB, resize if needed

    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
    stream.avail_in = compressed.size();
    stream.next_out = reinterpret_cast<Bytef*>(output.data());
    stream.avail_out = output.size();

    if (inflateInit(&stream) != Z_OK)
        throw std::runtime_error("inflateInit failed");

    int result = inflate(&stream, Z_FINISH);
    if (result != Z_STREAM_END)
        throw std::runtime_error("inflate failed");

    inflateEnd(&stream);
    return std::string(output.data(), stream.total_out);
}

std::string compressZlib(const std::string& input) {
    uLongf compressedSize = compressBound(input.size());
    std::vector<unsigned char> buffer(compressedSize);

    int result = compress2(
        buffer.data(),
        &compressedSize,
        reinterpret_cast<const Bytef*>(input.data()),
        input.size(),
        Z_BEST_COMPRESSION
    );

    if (result != Z_OK) {
        throw std::runtime_error("Compression failed");
    }

    return std::string(reinterpret_cast<char*>(buffer.data()), compressedSize);
}


std::string hash_sha1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];  // 20 bytes for SHA-1
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);

    std::ostringstream result;
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
        result << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return result.str();
}
