#include <coproto/Socket/LocalAsyncSock.h>
#include <cstdlib>
#include <future>
#include <iostream>
#include <shuffle.h>
#include <utlis.h>
#include "context.h"
int main(int argc, char *argv[])
{
    PsiAnalyticsContext pa(PA, atoi(argv[1]), atoi(argv[2]));
    PsiAnalyticsContext pb(PB, atoi(argv[1]), atoi(argv[2]));
    oc::Matrix<block> _b(atoi(argv[1]), 2 * atoi(argv[2]) + 1);
    for (int i = 0; i < _b.rows(); i++) {
        for (int j = 0; j < _b.cols(); j++) {
            _b(i, j) = block(i, j);
        }
    }
    for (auto i = 0; i < _b.rows(); i++) {
        for (auto j = 0; j < _b.cols(); j++) {
            std::cout << _b(i, j) << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "-----------------" << std::endl;

    auto p2 = std::async(std::launch::async, shuffle_receiver, std::ref(pb));
    auto p1 = std::async(std::launch::async, shuffle_sender, std::ref(_b), std::ref(pa));
    p1.get();
    auto [share, p] = p2.get();
    share += _b;

    std::cout << "-----------------" << std::endl;
    // for (auto i = 0; i < share.rows(); i++) {
    //     for (auto j = 0; j < share.cols(); j++) {
    //         std::cout << share(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }
    std::cout << "-----------------" << std::endl;
    // for (auto pp : p) {
    //     std::cout << pp << " ";
    // }
    // std::cout << std::endl;
}