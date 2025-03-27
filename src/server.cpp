#include <cassert>
#include <coproto/Common/span.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/MatrixView.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <libOTe/Tools/Coproto.h>
#include <vector>
#include "client.h"
#include "constants.h"
#include "context.h"
#include "joinData.h"
#include "lpn.h"
#include "opprf.h"
#include "oprf.h"
#include "shuffle.h"
#include "utlis.h"
void pb_map(
    const std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> &map,
    const Matrix &data,
    const Matrix &_b,
    const std::vector<block> &r1_,
    std::vector<block> &r1,
    std::vector<block> &key,
    oc::Matrix<block> &r2,
    const PsiAnalyticsContext &context)
{
    int index = 0;
    for (int i = 0; i < context.bins; i++) {
        auto it = map.find(i);
        if (it != map.end()) {
            auto &v = it->second;
            for (auto &[id_index, id] : v) {
                r1[index] = r1_[i];
                if (id_index != -1) {
                    key[index] = block(id, i);
                    for (size_t j = 0; j < context.pb_features; j++) {
                        r2(index, j) = data(id_index, j) - _b(i, j);
                    }
                } else {
                    // 随便塞点假值
                    key[index] = block(rand(), rand());
                    for (size_t j = 0; j < context.pb_features; j++) {
                        r2(index, j) = block(id, j);
                    }
                }
                index++;
            }
        }
    }
}

oc::Matrix<block> pb_share(PsiAnalyticsContext &context)
{
    oc::Matrix<block> outputs(context.bins, context.pa_features);
    MatrixRecv(outputs, context);

    return outputs;
}

// 使用简单哈希的一方
void server_run(PsiAnalyticsContext &context)
{
    context.setup();
    osuCrypto::Timer timer;
    context.setTimer(timer);
    timer.reset();
    timer.setTimePoint("start");

    joinData pb(context);
    timer.setTimePoint("init");

    oc::PRNG prng(block(rand(), rand()));
    auto map = SimpleHash(pb.ids, context);
    timer.setTimePoint("simple hash");

    std::vector<block> key(context.pb_elems * context.funcs);
    std::vector<block> r1(context.pb_elems * context.funcs);
    std::vector<block> r1_(context.bins);
    oc::Matrix<block> _b(context.bins, context.pb_features);
    prng.get(r1_.data(), context.bins);
    prng.get(_b.data(), context.bins * context.pb_features);
    oc::Matrix<block> r2(context.pb_elems * context.funcs, context.pb_features);
    prng.get(r2.data(), context.pb_elems * context.funcs * context.pb_features);
    pb_map(map, pb.features, _b, r1_, r1, key, r2, context);
    timer.setTimePoint("data prepare");

    opprfSender_1(key, r1, context);
    timer.setTimePoint("opprf 1st");

    opprfSender_2(key, r2, context);
    timer.setTimePoint("opprf 2nd");

    auto newID = oprfReceiver(r1_, context);
    timer.setTimePoint("oprf");

    Matrix pb_data;
    auto _a = pb_share(context);
    timer.setTimePoint("share");

    coproto::sync_wait(context.chl.flush());
    pb_data = mergeMatrix(newID, _a, _b, context);
    // std::cout << "finish merge\n";
    timer.setTimePoint("merge");

    // Shuffle
    if (!context.fake_offline) {
        shuffle_sender(pb_data, context);
        timer.setTimePoint("shuffle 1st");

        auto [pa_share, p] = shuffle_receiver(context);
        permuteMatrix(pb_data, p);
        pb_data += pa_share;
        timer.setTimePoint("shuffle 2nd");
    } else {
        size_t cols = (context.pa_features + context.pb_features + 1 ) * 2 ;
        Mat2d data(context.bins, cols), fake_mask(context.bins, cols), shuffle_mask1(context.bins, cols), shuffle_mask2(context.bins, cols);
        fake_mask.setZero();
        shuffle_mask1.setZero();
        shuffle_mask2.setZero();

        assert(pa_data.rows() == context.bins && pa_data.cols() == context.pa_features + context.pb_features + 1);

        for (size_t i = 0; i < context.bins; i++) {
            for (size_t j = 0; j < cols; ++j) {
                auto tmp = pb_data(i, j).mData;
                data(i, j) =
                    static_cast<uint64_t>(_mm_extract_epi64(tmp, 0)) - fake_mask(0, 1);
                ++j;
                data(i, j) =
                    static_cast<uint64_t>(_mm_extract_epi64(tmp, 0)) - fake_mask(0, 1);
            }
        }
        data -= fake_mask;
        send_array(data, context.socket_fd);
        data = shuffle_mask1;
        timer.setTimePoint("shuffle 1st");

        Mat2d tmp(context.bins, cols);
        recv_array(context.socket_fd, tmp);
        data += tmp;
        fixedShuffle(data, 1234);
        data += shuffle_mask2;
        timer.setTimePoint("shuffle 2nd");

        // Matrix fake_mask(context.bins, context.pa_features + context.pb_features + 1);
        // fake_mask = pb_data - fake_mask;
        // pb_data = fake_mask;
        // coproto::sync_wait(context.chl.send(fake_mask));
        // timer.setTimePoint("shuffle 1st");

        // coproto::sync_wait(context.chl.recv(fake_mask));
        // auto p = std::vector<uint64_t>(context.bins);
        // permuteMatrix(fake_mask, p);
        // pb_data += fake_mask;
        // timer.setTimePoint("shuffle 2nd");
    }
    // Trim
    std::vector<block> pa_res(context.bins);
    std::vector<block> pb_res(context.bins);
    for (int i = 0; i < context.bins; i++) {
        pb_res[i] = pb_data(i, 0);
    }
    coproto::sync_wait(context.chl.recv(pa_res));
    coproto::sync_wait(context.chl.send(pb_res));

    for (int i = 0; i < context.bins; i++) {
        if (pa_res[i] != pb_res[i]) {
            for (int j = 0; j < context.pa_features + context.pb_features + 1; j++) {
                pb_data(i, j) = block(0);
            }
        }
    }

    timer.setTimePoint("trim");
    timer.setTimePoint("total");
    context.close();
    context.print();
}

void server_test(PsiAnalyticsContext &context)
{
    context.setup();
    oc::PRNG prng(block(rand(), rand()));
    std::vector<block> key(context.pb_elems * context.funcs);
    prng.get(key.data(), context.pb_elems * context.funcs);
    oc::Matrix<block> r2(context.pb_elems * context.funcs, context.pb_features);
    prng.get(r2.data(), context.pb_elems * context.funcs * context.pb_features);
    opprfSender_2(key, r2, context);
}