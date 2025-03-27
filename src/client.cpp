#include "client.h"
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/MatrixView.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "constants.h"
#include "joinData.h"
#include "lpn.h"
#include "opprf.h"
#include "oprf.h"
#include "shuffle.h"
#include "utlis.h"
// 使用布谷鸟哈希的一方

Matrix pa_share(
    const Matrix &origin,
    PsiAnalyticsContext &context,
    std::map<uint64_t, std::pair<uint64_t, uint64_t>> map)
{
    oc::PRNG prng(block(rand(), rand()));
    Matrix share(context.bins, context.pa_features);
    prng.get(share.data(), share.size());
    MatrixSend(share, context);
    for (size_t i = 0; i < context.bins; i++) {
        auto it = map.find(i);
        if (it != map.end()) {
            auto k = it->second.first;
            for (size_t j = 0; j < context.pa_features; j++) {
                share(i, j) = origin(k, j) - share(i, j);
            }
        }
    }
    return share;
}

void client_run(PsiAnalyticsContext &context)
{
    context.setup();
    osuCrypto::Timer timer;
    context.setTimer(timer);
    timer.reset();
    timer.setTimePoint("start");

    joinData pa(context);
    oc::PRNG prng(block(3, 4));
    timer.setTimePoint("init");

    auto map = CuckooHash(pa.ids, context);
    timer.setTimePoint("cuckoo hash");

    std::vector<block> key(context.bins);
    prng.get(key.data(), context.bins);
    for (const auto &[l, id] : map) {
        key[l] = block(id.second, l);
    }
    timer.setTimePoint("data prepare");

    auto r1 = opprfReceiver_1(key, context);
    std::cout << "finish opprf 1st\n";
    timer.setTimePoint("opprf 1st");

    auto r2 = opprfReceiver_2(key, context);
    std::cout << "finish opprf 2nd\n";
    timer.setTimePoint("opprf 2nd");

    auto newID = oprfSender(r1, context);
    std::cout << "finish oprf\n";
    timer.setTimePoint("oprf");

    auto _a = pa_share(pa.features, context, map);
    std::cout << "finish share\n";
    timer.setTimePoint("share");

    coproto::sync_wait(context.chl.flush());
    Matrix pa_data = mergeMatrix(newID, _a, r2, context);
    std::cout << "finish merge\n";
    timer.setTimePoint("merge");

    // Shuffle
    if (!context.fake_offline) {
        auto [pa_data_, p] = shuffle_receiver(context);
        std::cout << "finish shuffle 1st\n";
        permuteMatrix(pa_data, p);
        matrixTransform(pa_data_, pa_data);
        timer.setTimePoint("shuffle 1st");

        shuffle_sender(pa_data_, context);
        std::cout << "finish shuffle 2nd\n";
        timer.setTimePoint("fake shuffle 2nd");
    } else {
        size_t cols = (context.pa_features + context.pb_features + 1) * 2;
        Mat2d data(context.bins, cols), fake_mask(context.bins, cols),
            shuffle_mask1(context.bins, cols), shuffle_mask2(context.bins, cols);
        fake_mask.setZero();
        shuffle_mask1.setZero();
        shuffle_mask2.setZero();

        assert(pa_data.rows() == context.bins && pa_data.cols() == context.pa_features + context.pb_features + 1);

        for (size_t i = 0; i < context.bins; i++) {
            for (size_t j = 0; j < cols; ++j) {
                auto tmp = pa_data(i, j).mData;
                data(i, j) = static_cast<uint64_t>(_mm_extract_epi64(tmp, 0)) - fake_mask(0, 1);
                ++j;
                data(i, j) = static_cast<uint64_t>(_mm_extract_epi64(tmp, 0)) - fake_mask(0, 1);
            }
        }

        Mat2d tmp(context.bins, cols);
        recv_array(context.socket_fd, tmp);
        data += tmp;
        fixedShuffle(data, 4567);
        data += shuffle_mask1;
        std::cout << "finish shuffle 1st\n";
        timer.setTimePoint("shuffle 1st");

        data -= fake_mask;
        send_array(data, context.socket_fd);
        data = shuffle_mask2;
        std::cout << "finish shuffle 2nd\n";
        timer.setTimePoint("shuffle 2nd");

        // Matrix fake_mask_s(context.bins, context.pa_features + context.pb_features + 1);
        // Matrix fake_mask_c(context.bins, context.pa_features + context.pb_features + 1);
        // coproto::sync_wait(context.chl.recv(fake_mask_s));
        // fake_mask_c += fake_mask_s;
        // auto p = std::vector<uint64_t>(context.bins);
        // permuteMatrix(fake_mask_c, p);
        // matrixTransform(pa_data, fake_mask_c);
        // std::cout << "finish fake shuffle 1st\n";
        // timer.setTimePoint("fake shuffle 1st");

        // coproto::sync_wait(context.chl.send(pa_data));
        // std::cout << "finish fake shuffle 2nd\n";
        // timer.setTimePoint("fake shuffle 2nd");
    }
    // Trim
    std::vector<block> pa_res(context.bins);
    std::vector<block> pb_res(context.bins);
    for (int i = 0; i < context.bins; i++) {
        pa_res[i] = pa_data(i, 0);
    }
    coproto::sync_wait(context.chl.send(pa_res));
    coproto::sync_wait(context.chl.recv(pb_res));
    for (int i = 0; i < context.bins; i++) {
        if (pa_res[i] != pb_res[i]) {
            for (int j = 0; j < context.pa_features + context.pb_features + 1; j++) {
                pa_data(i, j) = block(0);
            }
        }
    }
    timer.setTimePoint("trim");
    timer.setTimePoint("total");
    std::cout << timer;
    context.close();
    context.print();
}

void client_test(PsiAnalyticsContext &context)
{
    context.setup();
    oc::PRNG prng(block(3, 4));
    std::vector<block> key(context.bins);
    prng.get(key.data(), context.bins);
    auto r2 = opprfReceiver_2(key, context);
    std::cout << "finish\n";
}