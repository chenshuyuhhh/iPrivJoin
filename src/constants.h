#include <chrono>
#ifndef CONSTANTS
#define CONSTANTS

constexpr uint64_t __61_bit_mask = 0x1FFFFFFFFFFFFFFFull;
constexpr std::size_t symsecbits = 128;
using milliseconds_ratio = std::ratio<1, 1000>;
using duration_millis = std::chrono::duration<double, milliseconds_ratio>;
using GGMTreeNode = uint64_t;
using PRGSeed = uint64_t;
#endif