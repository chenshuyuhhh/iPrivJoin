#pragma once

#include <array>
#include <cryptoTools/Common/block.h>
#include <cstdint>
#include <future>
#include <string>
#include <vector>
#include "joinData.h"
using GGMTreeNode = uint64_t;
using PRGSeed = uint64_t;

class GGMTree {
public:
    std::vector<block> node;
    uint64_t high;

    GGMTree(const uint64_t &high);
    GGMTree(const uint64_t &high, const block key);
    GGMTree(
        const uint64_t &high,
        const std::vector<osuCrypto::block> &received,
        const std::string &choices);

    ~GGMTree() = default;
    void set(const uint64_t &index, const block &key, int depth = 0);
    block xorLeft(const uint64_t &layer) const;
    block xorRight(const uint64_t &layer) const;
    block xorLayer(const uint64_t &layer) const;

    std::vector<std::array<osuCrypto::block, 2>> eval() const;

    std::vector<block> leaf() const;

    void print() const;

private:
    std::vector<std::future<void>> futures;
    void setThreads(const uint64_t &index, const block &key);
    static constexpr int MAX_DEPTH_NO_THREADS = 5;
    bool check();
};

void gen(const PRGSeed &in, PRGSeed &first, PRGSeed &second);
void genXOR(const PRGSeed &in, PRGSeed &first, PRGSeed &second);
void ggm_send(const GGMTree &g, const std::string &address, PsiAnalyticsContext &context);
GGMTree ggm_recv(
    const uint64_t &path_num,
    const uint64_t &high,
    const std::string &address,
    PsiAnalyticsContext &context);
void printV(const std::vector<uint64_t> &v);
