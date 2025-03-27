#pragma once
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/block.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "context.h"
using block = osuCrypto::block;
using Matrix = osuCrypto::Matrix<block>;
template <typename T>
void to_block(const T &value, block &b)
{
    std::memset(&b, 0, sizeof(block));
    std::memcpy(&b, &value, (sizeof(T) > 16) ? 16 : sizeof(T));
}

inline void to_block(const std::string &value, block &b)
{
    std::memset(&b, 0, sizeof(block)); // Zero-initialize the block

    std::memcpy(&b, value.data(), (value.size() > 16) ? 16 : value.size());
}

template <typename T>
T from_block(const block &b)
{
    T value;
    std::memset(&value, 0, sizeof(T));
    std::memcpy(&value, &b, (sizeof(T) > 16) ? 16 : sizeof(T));
    return value;
}
template <>
inline std::string from_block(const block &b)
{
    std::string value(16, '\0');    // Create a string of 16 null characters
    std::memcpy(&value[0], &b, 16); // Copy the data from the block

    // Resize the string to remove null characters at the end
    value.resize(std::strlen(value.c_str()));

    return value;
}

class joinData;

class joinData {
public:
    joinData(const PsiAnalyticsContext &context);
    joinData(const joinData &) = default;
    joinData(joinData &&) = default;
    ~joinData() = default;

    joinData &operator=(const joinData &) = default;
    joinData &operator=(joinData &&) = default;

    void print();
    std::vector<uint64_t> ids;
    Matrix features;
};
