#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/block.h>
#include <vector>
#include "context.h"
using block = osuCrypto::block;

void opprfSender_1(
    const std::vector<block> &key,
    std::vector<block> &value,
    PsiAnalyticsContext &context);

std::vector<block> opprfReceiver_1(
    const std::vector<block> &key, PsiAnalyticsContext &context);

void opprfSender_2(
    const std::vector<block> &key,
    oc::Matrix<block> &value,
    PsiAnalyticsContext &context);

oc::Matrix<block> opprfReceiver_2(
    const std::vector<block> &key, PsiAnalyticsContext &context);