#include "shuffle.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Common/span.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Log.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <libOTe/Base/BaseOT.h>
#include <libOTe/Tools/Coproto.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Iknp/IknpOtExtSender.h>
#include <libOTe/TwoChooseOne/OTExtInterface.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>
#include <numeric>
#include <random>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>
#include <volePSI/Defines.h>
#include "../ggm/ggm.h"
#include "context.h"
#include "joinData.h"
#include "lpn.h"
#include "utlis.h"
// 生成随机vector

std::string incrementPort(const std::string &address, std::uint8_t offset);

template <typename T>
std::vector<T> generateRandomVector(const uint64_t &n, const uint64_t &seed)
{
    volePSI::PRNG rng({ seed, 0 });
    std::vector<T> vector(n);
    rng.get<T>(vector.data(), n);
    return vector;
}

template <>
std::vector<block> generateRandomVector(const uint64_t &n, const uint64_t &seed)
{
    volePSI::PRNG rng({ seed, 0 });
    std::vector<block> vector(n);
    rng.get<block>(vector.data(), n);
    return vector;
}

// 生成从0到n-1的排列
std::vector<uint64_t> generateRandomPermutation(const uint64_t &n, const int &seed)
{
    std::mt19937 rng(seed);
    std::vector<uint64_t> permutation(n);
    std::iota(permutation.begin(), permutation.end(), 0);
    std::shuffle(permutation.begin(), permutation.end(), rng);
    return permutation;
}

template <typename T>
void permuteVector(std::vector<T> &permutation, const std::vector<uint64_t> &pi)
{
    assert(permutation.size() == pi.size());
    std::vector<T> resultVector(permutation.size());
    std::transform(pi.begin(), pi.end(), resultVector.begin(), [&](uint64_t index) {
        return permutation[index];
    });
    permutation = std::move(resultVector);
}

template <typename T>
void xorVectors(std::vector<T> &v1, const std::vector<T> &v2)
{
    assert(v1.size() == v2.size());

    // 逐元素异或
    for (std::size_t i = 0; i < v1.size(); ++i) {
        v1[i] ^= v2[i];
    }
}

// 将两个维数相等的向量洗牌后逐元素异或，结果保存在第一个向量中
template <typename T>
void xorVectorsWithPI(std::vector<T> &v1, const std::vector<T> &v2, const std::vector<uint64_t> &pi)
{
    // 检查向量长度是否相同
    assert(v1.size() == v2.size());
    // 逐元素异或
    for (std::size_t i = 0; i < v1.size(); ++i) {
        v1[i] ^= v2[pi[i]];
    }
}

std::pair<block, std::vector<block>> mShuffleSend(
    const uint64_t &high, const block key, const std::string address, PsiAnalyticsContext &context)
{
    GGMTree g(high, key);
    ggm_send(g, address, context);
    return std::make_pair(g.xorLayer(high - 1), g.leaf());
}

std::vector<block> mShuffleSender(
    const std::vector<block> &x,
    const uint64_t &h,
    const std::string &address,
    PsiAnalyticsContext &context)
{
    uint64_t t = 1 << (h - 1);
    assert(x.size() == t);
    volePSI::PRNG prng({ 0, 0 });
    std::vector<block> b(t);
    std::vector<block> a(t, { 0, 0 });
    for (int i = 0; i < t; i++) {
        GGMTree g(h, prng.get());
        b.at(i) = g.xorLayer(h - 1);
        xorVectors(a, g.leaf());
        auto p = mShuffleSend(h, prng.get(), address, context);
        b.at(i) = p.first;
        xorVectors(a, p.second);
    }

    coproto::Socket chl = coproto::asioConnect(address, true);

    xorVectors(a, x);
    osuCrypto::cp::sync_wait(chl.send(a));
    osuCrypto::cp::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    osuCrypto::cp::sync_wait(chl.close());
    return b;
}

std::vector<block> mShuffleReceiver(
    const uint64_t &h,
    std::vector<uint64_t> &pi,
    const std::string &address,
    PsiAnalyticsContext &context)
{
    uint64_t t = 1 << (h - 1);
    std::vector<block> b(t);
    std::vector<block> a_pi(t, { 0, 0 });
    volePSI::PRNG prng({ 0, 0 });
    for (int i = 0; i < t; i++) {
        auto g = ggm_recv(pi[i], h, address, context);
        b.at(i) = g.xorLayer(h - 1);
        xorVectorsWithPI(a_pi, g.leaf(), pi);
    }
    xorVectors(a_pi, b);
    coproto::Socket chl = coproto::asioConnect(address, false);
    std::vector<block> xXORa(t, { 0, 0 });
    osuCrypto::cp::sync_wait(chl.recv(xXORa));
    osuCrypto::cp::sync_wait(chl.flush());
    context.totalReceive += chl.bytesReceived();
    context.totalSend += chl.bytesSent();
    osuCrypto::cp::sync_wait(chl.close());
    xorVectorsWithPI(a_pi, xXORa, pi);
    return a_pi;
}

std::vector<uint64_t> getFinalPI(
    std::vector<std::vector<uint64_t>> &share_pi,
    const std::vector<osuCrypto::BitVector> &permute_pi)
{
    std::vector<uint64_t> final_pi(share_pi[0].size());
    std::iota(final_pi.begin(), final_pi.end(), 0);
    for (int i = 0; i < share_pi.size(); i++) {
        for (int j = 0; j < permute_pi[i].size() / 2; j++) {
            if (permute_pi[i][j * 2] == 0) {
                std::swap(final_pi[j * 2], final_pi[j * 2 + 1]);
            }
        }
        permuteVector(final_pi, share_pi[i]);
    }
    for (int j = 0; j < permute_pi[0].size() / 2; j++) {
        if (permute_pi[permute_pi.size() - 1][j * 2] == 0) {
            std::swap(final_pi[j * 2], final_pi[j * 2 + 1]);
        }
    }
    return final_pi;
}

void shuffle_sender(Matrix &inputs, PsiAnalyticsContext &context)
{
    if (!context.setuped) {
        context.setup();
    }
    std::vector<std::vector<uint64_t>> share_pi(
        context.layers / 2, std::vector<uint64_t>(context.bins));
    for (auto &s : share_pi) {
        coproto::sync_wait(context.chl.recv(s));
    }
    oc::PRNG prng(block(0, 0));
    std::vector<block> mask(context.bins);
    prng.get(mask.data(), context.bins);
    auto expand = khprf(mask, context.J);
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            permute_layer_sender(mask, context);
        } else {
            permuteVector(mask, share_pi[i / 2]);
        }
    }
    auto mask2 = inputs - expand;
    osuCrypto::cp::sync_wait(context.chl.send(mask2));
    inputs = khprf(mask, context.J);
}

std::pair<Matrix, std::vector<uint64_t>> shuffle_receiver(PsiAnalyticsContext &context)
{
    if (!context.setuped) {
        context.setup();
    }
    std::vector<std::vector<uint64_t>> share_pi(
        context.layers / 2, std::vector<uint64_t>(context.bins));
    std::vector<osuCrypto::BitVector> permute_pi(context.layers - (context.layers / 2));
    std::mt19937 rng(110);
    osuCrypto::PRNG prng(block(0, 0));
    for (auto &s : share_pi) {
        std::iota(s.begin(), s.end(), 0);
        std::shuffle(s.begin(), s.end(), rng);
        coproto::sync_wait(context.chl.send(s));
    }

    for (auto &p : permute_pi) {
        for (int i = 0; i < context.bins / 2; i++) {
            if (prng.get<bool>()) {
                p.pushBack(1);
                p.pushBack(0);
            } else {
                p.pushBack(0);
                p.pushBack(1);
            }
        }
    }
    auto final_pi = getFinalPI(share_pi, permute_pi);
    std::vector<block> mask(context.bins);
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            auto temp = permute_layer_receiver(context, permute_pi[i / 2]);
            for (int j = 0; j < context.bins / 2; j++) {
                if (permute_pi[i / 2][2 * j] == 0) {
                    std::swap(mask[2 * j], mask[2 * j + 1]);
                }
                mask[2 * j] += temp[2 * j];
                mask[2 * j + 1] += temp[2 * j + 1];
            }
        } else {
            permuteVector(mask, share_pi[i / 2]);
        }
    }

    Matrix mask2(context.bins, context.pb_features + context.pa_features + 1);
    osuCrypto::cp::sync_wait(context.chl.recv(mask2));
    permuteMatrix(mask2, final_pi);
    return std::make_pair(mask2 + khprf(mask, context.J), std::move(final_pi));
}

void ot_test_client(int bins, coproto::Socket chl)
{
    osuCrypto::Timer timer;

    osuCrypto::BitVector bv;
    osuCrypto::PRNG prng(block(0, 0));
    for (int i = 0; i < bins / 2; i++) {
        if (prng.get<bool>()) {
            bv.pushBack(1);
            bv.pushBack(0);
        } else {
            bv.pushBack(0);
            bv.pushBack(1);
        }
    }
    timer.setTimePoint("start");
    {
        osuCrypto::DefaultBaseOT recver;
        std::vector<block> outputs(bins);
        coproto::sync_wait(recver.receiveChosen(bv, outputs, prng, chl));
    }
    timer.setTimePoint("Base");

    {
        osuCrypto::IknpOtExtReceiver recver;
        std::vector<block> outputs(bins);
        coproto::sync_wait(recver.genBaseOts(prng, chl));
        coproto::sync_wait(recver.receiveChosen(bv, outputs, prng, chl));
    }
    timer.setTimePoint("Iknp");

    {
        osuCrypto::SilentOtExtReceiver recver;
        std::vector<block> outputs(bins);
        recver.configure(bins);
        coproto::sync_wait(recver.genSilentBaseOts(prng, chl));
        coproto::sync_wait(recver.receiveChosen(bv, outputs, prng, chl));
    }
    timer.setTimePoint("Silent");
    std::cout << timer;
}

void ot_test_server(int bins, coproto::Socket chl)
{
    osuCrypto::PRNG prng(block(0, 0));
    std::vector<std::array<block, 2>> inputs(bins);
    prng.get(inputs.data(), bins);
    {
        osuCrypto::DefaultBaseOT sender;
        coproto::sync_wait(sender.sendChosen(inputs, prng, chl));
    }
    {
        osuCrypto::IknpOtExtSender sender;
        coproto::sync_wait(sender.genBaseOts(prng, chl));
        coproto::sync_wait(sender.sendChosen(inputs, prng, chl));
    }
    {
        osuCrypto::SilentOtExtSender sender;
        sender.configure(bins);
        coproto::sync_wait(sender.genSilentBaseOts(prng, chl));
        coproto::sync_wait(sender.sendChosen(inputs, prng, chl));
    }
}

std::vector<block> permute_layer_receiver(
    PsiAnalyticsContext &context, const osuCrypto::BitVector &choices)
{
    int bins_even = context.bins / 2 * 2;
    osuCrypto::PRNG prng(block(0, 0));
    std::vector<block> ot_message(bins_even);
    osuCrypto::SilentOtExtReceiver receiver;
    receiver.configure(bins_even);
    coproto::sync_wait(receiver.genSilentBaseOts(prng, context.chl));
    coproto::sync_wait(receiver.receiveChosen(choices, ot_message, prng, context.chl));
    std::vector<block> mask(bins_even);
    coproto::sync_wait(context.chl.recv(mask));
    std::vector<block> outputs(bins_even);
    for (int i = 0; i < bins_even / 2; i++) {
        if (choices[i * 2] == 1) {
            outputs[i * 2] = mask[i * 2] + ot_message[i * 2 + 1] - ot_message[i * 2];
            outputs[i * 2 + 1] = mask[i * 2 + 1] + ot_message[i * 2] - ot_message[i * 2 + 1];
        } else {
            outputs[i * 2] = mask[i * 2 + 1] + ot_message[i * 2 + 1] - ot_message[i * 2];
            outputs[i * 2 + 1] = mask[i * 2] + ot_message[i * 2] - ot_message[i * 2 + 1];
        }
    }
    return outputs;
}

void permute_layer_sender(std::vector<block> &inputs, PsiAnalyticsContext &context)
{
    int bins_even = context.bins / 2 * 2;
    osuCrypto::PRNG prng(block(0, 0));

    std::vector<std::array<block, 2>> ot_message(bins_even);
    std::vector<block> mask(bins_even);
    prng.get(ot_message.data(), bins_even);
    std::vector<block> b(bins_even);
    auto xorfunc = [&ot_message, &inputs](std::vector<block> &b, std::vector<block> &mask) {
        for (int i = 0; i < ot_message.size() / 2; ++i) {
            b[i * 2] = ot_message[i * 2][0] + ot_message[i * 2][1];
            b[i * 2 + 1] = ot_message[i * 2 + 1][0] + ot_message[i * 2 + 1][1];
            mask[i * 2] = inputs[i * 2] - ot_message[i * 2][0] - ot_message[i * 2 + 1][0];
            mask[i * 2 + 1] = inputs[i * 2 + 1] - ot_message[i * 2][1] - ot_message[i * 2 + 1][1];
        }
    };
    auto xor_feature = std::async(std::launch::async, xorfunc, std::ref(b), std::ref(mask));
    osuCrypto::SilentOtExtSender sender;
    sender.configure(bins_even);

    coproto::sync_wait(sender.genSilentBaseOts(prng, context.chl));
    coproto::sync_wait(sender.sendChosen(ot_message, prng, context.chl));
    xor_feature.get();
    coproto::sync_wait(context.chl.send(mask));
    inputs = b;
}

void shuffle_sender2(Matrix &inputs, PsiAnalyticsContext &context)
{
    if (!context.setuped) {
        context.setup();
    }
    osuCrypto::Timer timer;
    context.setTimer(timer);
    timer.reset();
    timer.setTimePoint("start");
    std::vector<std::vector<uint64_t>> share_pi(
        context.layers / 2, std::vector<uint64_t>(context.bins));
    for (auto &s : share_pi) {
        coproto::sync_wait(context.chl.recv(s));
    }
    coproto::sync_wait(context.chl.flush());

    oc::PRNG prng(block(0, 0));
    std::vector<block> mask(context.bins);
    prng.get(mask.data(), context.bins);
    // auto expand = khprf(mask, context.J);
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            permute_layer_sender(mask, context);
        } else {
            permuteVector(mask, share_pi[i / 2]);
        }
    }
    coproto::sync_wait(context.chl.flush());
    timer.setTimePoint("shuffle");
    std::cout << timer;
    context.close();
    context.print();
    return;
    // auto mask2 = inputs - expand;
    // osuCrypto::cp::sync_wait(context.chl.send(mask2));
    // inputs = khprf(mask, context.J);
}

void shuffle_receiver2(PsiAnalyticsContext &context)
{
    if (!context.setuped) {
        context.setup();
    }
    osuCrypto::Timer timer;
    context.setTimer(timer);
    timer.reset();
    timer.setTimePoint("start");
    std::vector<std::vector<uint64_t>> share_pi(
        context.layers / 2, std::vector<uint64_t>(context.bins));
    std::vector<osuCrypto::BitVector> permute_pi(context.layers - (context.layers / 2));
    std::mt19937 rng(110);
    osuCrypto::PRNG prng(block(0, 0));
    for (auto &s : share_pi) {
        std::iota(s.begin(), s.end(), 0);
        std::shuffle(s.begin(), s.end(), rng);
        coproto::sync_wait(context.chl.send(s));
    }
    coproto::sync_wait(context.chl.flush());
    for (auto &p : permute_pi) {
        for (int i = 0; i < context.bins / 2; i++) {
            if (prng.get<bool>()) {
                p.pushBack(1);
                p.pushBack(0);
            } else {
                p.pushBack(0);
                p.pushBack(1);
            }
        }
    }
    auto final_pi = getFinalPI(share_pi, permute_pi);
    std::vector<block> mask(context.bins);
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            auto temp = permute_layer_receiver(context, permute_pi[i / 2]);
            for (int j = 0; j < context.bins / 2; j++) {
                if (permute_pi[i / 2][2 * j] == 0) {
                    std::swap(mask[2 * j], mask[2 * j + 1]);
                }
                mask[2 * j] += temp[2 * j];
                mask[2 * j + 1] += temp[2 * j + 1];
            }
        } else {
            permuteVector(mask, share_pi[i / 2]);
        }
    }
    timer.setTimePoint("shuffle");
    std::cout << timer;
    context.close();
    context.print();
    // return mask;
    // return;
    // Matrix mask2(context.bins, context.pb_features + context.pa_features);
    // osuCrypto::cp::sync_wait(context.chl.recv(mask2));
    // permuteMatrix(mask2, final_pi);
    // return std::make_pair(mask2 + khprf(mask, context.J), std::move(final_pi));
}

void shuffle_sendermul(PsiAnalyticsContext &context)
{
    if (!context.setuped) {
        context.setup();
    }
    osuCrypto::Timer timer;
    context.setTimer(timer);
    timer.reset();
    timer.setTimePoint("start");
    std::vector<std::vector<uint64_t>> share_pi(
        context.layers / 2, std::vector<uint64_t>(context.bins));
    for (auto &s : share_pi) {
        coproto::sync_wait(context.chl.recv(s));
    }
    coproto::sync_wait(context.chl.flush());

    oc::PRNG prng(block(0, 0));
    size_t cols = context.pa_features + context.pb_features + 1;
    std::vector<block> *mask = new std::vector<block>[cols];
    for (size_t j = 0; j < cols; j++) {
        mask[j] = std::vector<block>(context.bins);
        prng.get<block>(mask[j]);
    }
    // auto expand = khprf(mask, context.J);
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            for (size_t j = 0; j < cols; j++) {
                permute_layer_sender(mask[j], context);
            }
        } else {
            for (size_t j = 0; j < cols; j++) {
                permuteVector(mask[j], share_pi[i / 2]);
            }
        }
    }
    timer.setTimePoint("shuffle");
    std::cout << timer;
    context.close();
    context.print();
    return;
    // auto mask2 = inputs - expand;
    // osuCrypto::cp::sync_wait(context.chl.send(mask2));
    // inputs = khprf(mask, context.J);
}

void shuffle_receivermul(PsiAnalyticsContext &context)
{
    if (!context.setuped) {
        context.setup();
    }
    osuCrypto::Timer timer;
    context.setTimer(timer);
    timer.reset();
    timer.setTimePoint("start");
    std::vector<std::vector<uint64_t>> share_pi(
        context.layers / 2, std::vector<uint64_t>(context.bins));
    std::vector<osuCrypto::BitVector> permute_pi(context.layers - (context.layers / 2));
    std::mt19937 rng(110);
    osuCrypto::PRNG prng(block(0, 0));
    for (auto &s : share_pi) {
        std::iota(s.begin(), s.end(), 0);
        std::shuffle(s.begin(), s.end(), rng);
        coproto::sync_wait(context.chl.send(s));
    }
    coproto::sync_wait(context.chl.flush());

    size_t cols = context.pa_features + context.pb_features + 1;
    for (auto &p : permute_pi) {
        for (int i = 0; i < context.bins / 2; i++) {
            if (prng.get<bool>()) {
                p.pushBack(1);
                p.pushBack(0);
            } else {
                p.pushBack(0);
                p.pushBack(1);
            }
        }
    }
    auto final_pi = getFinalPI(share_pi, permute_pi);
    std::vector<block> *masks = new std::vector<block>[cols];
    for (size_t j = 0; j < cols; j++) {
        masks[j] = std::vector<block>(context.bins);
    }
    for (int i = 0; i < context.layers; i++) {
        if (i % 2 == 0) {
            for (size_t k = 0; k < cols; k++) {
                auto mask = masks[k];
                auto temp = permute_layer_receiver(context, permute_pi[i / 2]);
                for (int j = 0; j < context.bins / 2; j++) {
                    if (permute_pi[i / 2][2 * j] == 0) {
                        std::swap(mask[2 * j], mask[2 * j + 1]);
                    }
                    mask[2 * j] += temp[2 * j];
                    mask[2 * j + 1] += temp[2 * j + 1];
                }
            }
        } else {
            for (size_t k = 0; k < cols; k++) {
                auto mask = masks[k];
                permuteVector(mask, share_pi[i / 2]);
            }
        }
    }
    timer.setTimePoint("shuffle");
    std::cout << timer;
    context.close();
    context.print();
    // return mask;
    // return;
    // Matrix mask2(context.bins, context.pb_features + context.pa_features);
    // osuCrypto::cp::sync_wait(context.chl.recv(mask2));
    // permuteMatrix(mask2, final_pi);
    // return std::make_pair(mask2 + khprf(mask, context.J), std::move(final_pi));
}