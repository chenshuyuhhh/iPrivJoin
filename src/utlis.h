#include <arpa/inet.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/block.h>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <unistd.h>
#include <vector>
#include "context.h"
#include "joinData.h"
using block = osuCrypto::block;
void PrintInfo(const PsiAnalyticsContext &context);
std::vector<uint64_t> GetHashTableThroughFile(const std::string filePath);
std::map<uint64_t, std::pair<uint64_t, uint64_t>> CuckooHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context);
std::map<uint64_t, std::vector<std::pair<uint64_t, uint64_t>>> SimpleHash(
    const std::vector<uint64_t> &ids, PsiAnalyticsContext &context);
Matrix mergeMatrix(
    const std::vector<block> ids, const Matrix &a, const Matrix &b, PsiAnalyticsContext &context);
std::vector<block> minus(const std::vector<block> &a, const oc::span<block> &b);
std::vector<block> minus(const std::vector<block> &a, const std::vector<block> &b);
std::vector<block> minus(const oc::span<block> &a, const oc::span<block> &b);
Matrix operator+(const Matrix &a, const Matrix &b);
Matrix operator-(const Matrix &a, const Matrix &b);
void operator+=(Matrix &a, const Matrix &b);
void operator-=(Matrix &a, const Matrix &b);
void permuteMatrix(Matrix &a, const std::vector<size_t> &permute);
void matrixTransform(Matrix &a, const Matrix &b);
void MatrixRecv(Matrix &result, PsiAnalyticsContext &context);
void MatrixSend(const Matrix &value, PsiAnalyticsContext &context);
void fixedShuffle(std::vector<int> &indices, int rand);
void fixedShuffle(Mat2d &mat, int rand);
void fixedShuffle(Mat2d &mat, std::vector<int> &indices);