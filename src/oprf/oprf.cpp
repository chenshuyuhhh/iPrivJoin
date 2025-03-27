#include "oprf.h"
#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <cryptoTools/Network/Channel.h>
#include <vector>
#include <volePSI/RsOprf.h>

std::vector<block> oprfSender(const std::vector<block> &inputs, PsiAnalyticsContext &context)
{
    oc::PRNG prng(block(0, 0));
    volePSI::RsOprfSender sender;
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    uint64_t offset = 0;
    uint64_t length = context.bins;
    std::vector<oc::block> result(context.bins);
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        auto p = sender.send(chunkSize, prng, context.chl, context.threads);
        coproto::sync_wait(p);

        osuCrypto::span<const block> inputSpan(inputs.data() + offset, chunkSize);
        osuCrypto::span<block> resultSpan(result.data() + offset, chunkSize);
        sender.eval(inputSpan, resultSpan);
        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(context.chl.flush());
    return result;
}

std::vector<block> oprfReceiver(const std::vector<block> &inputs, PsiAnalyticsContext &context)
{
    std::vector<oc::block> result(context.bins);

    oc::PRNG prng(oc::block(0, 0));
    volePSI::RsOprfReceiver receiver;
    uint64_t offset = 0;
    uint64_t length = context.bins;
    const uint64_t maxChunkSize = std::numeric_limits<uint32_t>::max() / 16;
    while (length > 0) {
        uint64_t chunkSize = std::min(length, maxChunkSize);
        osuCrypto::span<const block> inputSpan(inputs.data() + offset, chunkSize);
        osuCrypto::span<block> resultSpan(result.data() + offset, chunkSize);
        auto p0 = receiver.receive(inputSpan, resultSpan, prng, context.chl, context.threads);
        coproto::sync_wait(p0);

        offset += chunkSize;
        length -= chunkSize;
    }
    coproto::sync_wait(context.chl.flush());

    return result;
}
