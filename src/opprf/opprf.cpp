#include "opprf.h"
#include <cassert>
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Log.h>
#include <cryptoTools/Common/MatrixView.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Network/Channel.h>
#include <cstdint>
#include <limits>
#include <vector>
#include <volePSI/RsOpprf.h>
#include "utlis.h"

#define lock osuCrypto::IoStream::lock
#define unlock osuCrypto::IoStream::unlock

void opprfSender_1(
    const std::vector<block> &key, std::vector<block> &value, PsiAnalyticsContext &context)
{
    assert(
        value.size() == context.pb_elems * context.funcs &&
        key.size() == context.pb_elems * context.funcs);
    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    auto p = sender.send(context.bins, key, value, prng, context.threads, context.chl);
    coproto::sync_wait(p);
    coproto::sync_wait(context.chl.flush());
}

std::vector<block> opprfReceiver_1(const std::vector<block> &key, PsiAnalyticsContext &context)
{
    assert(key.size() == context.bins);
    std::vector<block> outputs(context.bins);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));
    auto p = receiver.receive(
        context.pb_elems * context.funcs, key, outputs, prng, context.threads, context.chl);
    coproto::sync_wait(p);
    coproto::sync_wait(context.chl.flush());

    return outputs;
}

void opprfSender_2(
    const std::vector<block> &key, oc::Matrix<block> &value, PsiAnalyticsContext &context)
{
    assert(
        value.rows() == context.pb_elems * context.funcs && value.cols() == context.pb_features &&
        key.size() == context.pb_elems * context.funcs);
    volePSI::RsOpprfSender sender;
    oc::PRNG prng(block(0, 0));
    auto p = sender.send(context.bins, key, value, prng, context.threads, context.chl);
    coproto::sync_wait(p);
    coproto::sync_wait(context.chl.flush());
}

oc::Matrix<block> opprfReceiver_2(const std::vector<block> &key, PsiAnalyticsContext &context)
{
    assert(key.size() == context.bins);
    oc::Matrix<block> outputs(context.bins, context.pb_features);
    volePSI::RsOpprfReceiver receiver;
    oc::PRNG prng(block(0, 0));
    auto p = receiver.receive(
        context.pb_elems * context.funcs, key, outputs, prng, context.threads, context.chl);
    coproto::sync_wait(p);
    coproto::sync_wait(context.chl.flush());

    return outputs;
}
