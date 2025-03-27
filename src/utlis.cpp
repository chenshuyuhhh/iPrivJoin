#include "utlis.h"
#include <algorithm>
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/config.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Session.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <kuku/common.h>
#include <kuku/kuku.h>
#include <kuku/locfunc.h>
#include <libOTe/Tools/Coproto.h>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "constants.h"
#include "context.h"
#include "joinData.h"

void operator-=(Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            a(i, j) -= b(i, j);
        }
    }
}

Matrix operator+(const Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    Matrix result(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            result(i, j) = a(i, j) + b(i, j);
        }
    }
    return result;
}

Matrix operator-(const Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    Matrix result(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            result(i, j) = a(i, j) - b(i, j);
        }
    }
    return result;
}

void operator+=(Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            a(i, j) += b(i, j);
        }
    }
}

void matrixTransform(Matrix &a, const Matrix &b)
{
    assert(a.rows() == b.rows() && a.cols() == b.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        a(i, 0) -= b(i, 0);
        for (size_t j = 1; j < a.cols(); j++) {
            a(i, j) += b(i, j);
        }
    }
}

void permuteMatrix(Matrix &a, const std::vector<size_t> &permute)
{
    assert(a.rows() == permute.size());
    Matrix temp(a.rows(), a.cols());
    for (size_t i = 0; i < a.rows(); i++) {
        for (size_t j = 0; j < a.cols(); j++) {
            temp(i, j) = a(permute[i], j);
        }
    }
    a = std::move(temp);
}

std::map<uint64_t, std::pair<uint64_t, uint64_t>> CuckooHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context)
{
    kuku::KukuTable table(context.bins, 0, context.funcs, { 0, 0 }, 90000, kuku::make_item(0, 0));
    std::for_each(ids.begin(), ids.end(), [&table](const uint64_t &v) {
        if (!table.insert(kuku::make_item(v, 0))) {
            std::cout << "fill rate: " << table.fill_rate() << std::endl;
            std::cout << "failed item: " << v << std::endl;
            std::cout << "failed in kuku table" << std::endl;
            throw "failed in kuku table";
        }
    });
    std::cout << osuCrypto::IoStream::lock;
    std::cout << "kuku hash success with fill rate: " << table.fill_rate() << std::endl;
    std::cout << osuCrypto::IoStream::unlock;
    std::map<uint64_t, std::pair<uint64_t, uint64_t>> loc_id_map;
    for (size_t i = 0; i < ids.size(); i++) {
        kuku::QueryResult res = table.query(kuku::make_item(ids[i], 0));
        loc_id_map[res.location()] = std::make_pair(i, ids[i]);
    }
    std::cout << osuCrypto::IoStream::lock;
    std::cout << "finish cuckoo hash\n";
    std::cout << osuCrypto::IoStream::unlock;
    return loc_id_map;
}

std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> SimpleHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context)
{
    oc::PRNG prng(block(context.seedJ - 1));
    std::vector<kuku::LocFunc> funcs;
    for (size_t i = 0; i < context.funcs; i++) {
        funcs.emplace_back(context.bins, kuku::make_item(i, 0));
    }
    // 分别代表：桶下标、id下标、id值
    std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> loc_index_id_map;

    for (size_t i = 0; i < ids.size(); i++) {
        std::set<uint64_t> sets = std::set<uint64_t>();
        for (auto f : funcs) {
            uint64_t loc = f(kuku::make_item(ids[i], 0));
            //! 偶尔会出现hash到一个桶里的情况，这回导致opprf失效，这里直接跳过
            // 现在不能直接跳过，转而生成一个随机元素放入桶中，并标注下标为-1
            if (!sets.insert(loc).second) {
                loc_index_id_map.at(loc).push_back(std::make_pair(-1, prng.get()));
                continue;
            }
            if (loc_index_id_map.find(loc) != loc_index_id_map.end()) {
                loc_index_id_map.at(loc).push_back(std::make_pair(i, ids[i]));
            } else {
                std::vector<std::pair<uint64_t, uint64_t>> temp;
                temp.push_back(std::make_pair(i, ids[i]));
                loc_index_id_map[loc] = std::move(temp);
            }
        }
    }
    std::cout << osuCrypto::IoStream::lock;
    std::cout << "finish simple hash\n";
    std::cout << osuCrypto::IoStream::unlock;
    return loc_index_id_map;
}
std::vector<block> minus(const std::vector<block> &a, const oc::span<block> &b)
{
    assert(a.size() == b.size());
    std::vector<block> result(a.size());
    std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<block>());
    return result;
}

std::vector<block> minus(const oc::span<block> &a, const oc::span<block> &b)
{
    assert(a.size() == b.size());
    std::vector<block> result(a.size());
    std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<block>());
    return result;
}

std::vector<block> minus(const std::vector<block> &a, const std::vector<block> &b)
{
    assert(a.size() == b.size());
    std::vector<block> result(a.size());
    std::transform(a.begin(), a.end(), b.begin(), result.begin(), std::minus<block>());
    return result;
}

Matrix mergeMatrix(
    const std::vector<block> ids, const Matrix &a, const Matrix &b, PsiAnalyticsContext &context)
{
    assert(a.rows() == b.rows() && a.rows() == ids.size());
    Matrix v(context.bins, a.cols() + b.cols() + 1, osuCrypto::AllocType::Zeroed);
    for (size_t i = 0; i < a.rows(); i++) {
        v(i, 0) = ids[i];
        for (size_t j = 0; j < a.cols(); j++) {
            v(i, j + 1) = a(i, j);
        }
        for (size_t j = 0; j < b.cols(); j++) {
            v(i, j + a.cols() + 1) = b(i, j);
        }
    }
    return v;
}

void minus(std::vector<std::vector<block>> &a, const std::vector<std::vector<block>> &b)
{
    assert(a.size() == b.size());
    assert(a[0].size() == b[0].size());
    for (size_t i = 0; i < a.size(); i++) {
        for (size_t j = 0; j < a[i].size(); j++) {
            a[i][j] -= b[i][j];
        }
    }
}

void PrintInfo(const PsiAnalyticsContext &context)
{
    // std::cout << "Time for wait " << context.timings.wait << "ms\n";
    // std::cout << "Time for hashing " << context.timings.hashing << " ms\n";
    // std::cout << "Time for OPRF " << context.timings.oprf1st << " ms\n";
    // std::cout << "Time for polynomials " << context.timings.polynomials << " ms\n";
    // std::cout << "Time for transmission of the polynomials "
    //           << context.timings.polynomials_transmission << " ms\n";
    // std::cout << "Total runtime: " << context.timings.total << "ms\n";
    // std::cout << "Total runtime-wait time: " << context.timings.total - context.timings.wait
    //           << "ms\n";
    // std::cout << "Total receive: " << context.totalReceive * 1.0 / 1048576 << "MB\n";
    // std::cout << "Total send: " << context.totalSend * 1.0 / 1048576 << "MB\n";
}

void MatrixRecv(Matrix &result, PsiAnalyticsContext &context)
{
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = result.size();
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        oc::span<block> resultSpan(result.data() + offset, chunkSize);
        auto p = context.chl.recv(resultSpan);
        coproto::sync_wait(p);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(context.chl.flush());
}

void MatrixSend(const Matrix &value, PsiAnalyticsContext &context)
{
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = value.size();
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        osuCrypto::span<block> shareSpan(value.data() + offset, chunkSize);
        auto p = context.chl.send(shareSpan);
        coproto::sync_wait(p);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(context.chl.flush());
}


void fixedShuffle(Mat2d &mat, int rand)
{
    std::mt19937 gen(rand); // Use rand() as the seed
    std::vector<int> indices(mat.rows());
    
    // Initialize indices
    for (int i = 0; i < mat.rows(); ++i) {
        indices[i] = i;
    }
    
    // Shuffle the indices
    std::shuffle(indices.begin(), indices.end(), gen);
    
    // Create a temporary matrix to store the shuffled rows
    Mat2d tempMat = mat;
    
    // Reorder the rows based on the shuffled indices
    for (int i = 0; i < mat.rows(); ++i) {
        mat.row(i) = tempMat.row(indices[i]);
    }
}