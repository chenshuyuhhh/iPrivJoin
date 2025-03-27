#pragma once

#include <cryptoTools/Common/BitVector.h>
#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>
#include "joinData.h"
std::vector<uint64_t> generateRandomPermutation(const uint64_t &n, const int &seed);
template <typename T>
std::vector<T> generateRandomVector(const uint64_t &n, const uint64_t &seed);
template <typename T>
void permuteVector(std::vector<T> &permutation, const std::vector<uint64_t> &pi);
template <typename T>
void xorVectors(std::vector<T> &v1, const std::vector<T> &v2);
template <typename T>
void xorVectorsWithPI(
    std::vector<T> &v1, const std::vector<T> &v2, const std::vector<uint64_t> &pi);
std::vector<block> mShuffleReceiver(
    const uint64_t &h,
    std::vector<uint64_t> &pi,
    const std::string &address,
    PsiAnalyticsContext &context);
std::vector<block> mShuffleSender(
    const std::vector<block> &x,
    const uint64_t &h,
    const std::string &address,
    PsiAnalyticsContext &context);

void shuffle_sender(Matrix &inputs, PsiAnalyticsContext &context);
std::pair<Matrix, std::vector<uint64_t>> shuffle_receiver(PsiAnalyticsContext &context);

void shuffle_sender2(Matrix &inputs, PsiAnalyticsContext &context);
void shuffle_receiver2(PsiAnalyticsContext &context);

void shuffle_sendermul(PsiAnalyticsContext &context);
void shuffle_receivermul(PsiAnalyticsContext &context);


void ot_test_client(int bins, coproto::Socket chl);
void ot_test_server(int bins, coproto::Socket chl);

void permute_layer_sender(std::vector<block> &inputs, PsiAnalyticsContext &context);
std::vector<block> permute_layer_receiver(
    PsiAnalyticsContext &context, const osuCrypto::BitVector &choices);
