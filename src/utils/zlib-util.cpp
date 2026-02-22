#include <vector>
#include <string>
#include <stdexcept>
#include "../headers/ZlibUtils.hpp"
#include <zlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <cmath>
#include <cstdint>

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
    auto leftrotate = [](uint32_t value, int bits) -> uint32_t {
        return (value << bits) | (value >> (32 - bits));
    };

    std::vector<unsigned char> msg(data.begin(), data.end());
    uint64_t bit_len = static_cast<uint64_t>(msg.size()) * 8;

    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<unsigned char>((bit_len >> (i * 8)) & 0xFF));
    }

    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            size_t j = chunk + i * 4;
            w[i] = (static_cast<uint32_t>(msg[j]) << 24) |
                   (static_cast<uint32_t>(msg[j + 1]) << 16) |
                   (static_cast<uint32_t>(msg[j + 2]) << 8) |
                   static_cast<uint32_t>(msg[j + 3]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = leftrotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f = 0;
            uint32_t k = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = leftrotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = leftrotate(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    uint32_t digest[5] = {h0, h1, h2, h3, h4};
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (int i = 0; i < 5; ++i) {
        result << std::setw(8) << digest[i];
    }
    return result.str();
}



std::string getCurrentTimestampWithTimezone() {
    std::time_t now = std::time(nullptr);

    // Get local and UTC times
    std::tm localTime = *std::localtime(&now);
    std::tm gmTime = *std::gmtime(&now);

    // Calculate timezone offset in minutes
    int hourDiff = localTime.tm_hour - gmTime.tm_hour;
    int minDiff = localTime.tm_min - gmTime.tm_min;
    int totalMinutes = hourDiff * 60 + minDiff;

    // Adjust if crossing midnight boundary
    if (localTime.tm_mday != gmTime.tm_mday) {
        int dayDiff = localTime.tm_mday - gmTime.tm_mday;
        totalMinutes += dayDiff * 24 * 60;
    }

    // Format timezone as +0530
    char tzBuffer[6];
    std::snprintf(tzBuffer, sizeof(tzBuffer), "%+03d%02d", totalMinutes / 60, std::abs(totalMinutes % 60));

    // Build final output string
    std::ostringstream result;
    result << now << " " << tzBuffer;
    return result.str(); // e.g. "1718945703 +0530"
}


std::string hexToBinary(const std::string& hex) {
    if (hex.length() != 40 || (hex.length() % 2) != 0) {
        throw std::invalid_argument("Hex string must be 40 characters for SHA-1");
    }

    std::string binary;
    binary.reserve(20);

    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteStr, nullptr, 16));
        binary.push_back(static_cast<char>(byte));
    }

    return binary;
}

std::string binaryToHex(const std::string& binary) {
    if (binary.size() != 20) {
        throw std::invalid_argument("Binary input must be 20 bytes for SHA-1");
    }

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (unsigned char byte : binary) {
        hex << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return hex.str();
}
