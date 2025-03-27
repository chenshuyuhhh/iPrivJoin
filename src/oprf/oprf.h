#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/block.h>
#include <vector>
#include "context.h"
using block = osuCrypto::block;
std::vector<block> oprfSender(
    const std::vector<block> &inputs, PsiAnalyticsContext &context);
std::vector<block> oprfReceiver(
    const std::vector<block> &inputs, PsiAnalyticsContext &context);
