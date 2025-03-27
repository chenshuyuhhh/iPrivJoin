#include <coproto/Socket/LocalAsyncSock.h>
#include <cryptoTools/Common/Log.h>
#include <volePSI/Defines.h>
#include <filesystem>
#include <cstdlib>
#include <future>
#include <iostream>
#include <shuffle.h>
#include <utlis.h>
#include "context.h"
#include <vector>

int main(int argc, char *argv[])
{
    std::string config_file = argv[1];
    std::string env = argv[2];

    std::filesystem::path config_path(config_file);
    std::string task_name = config_path.stem().string();
    std::cout << osuCrypto::IoStream::lock;
    std::cout << config_file << std::endl;
    std::cout << osuCrypto::IoStream::unlock;
    std::string output_filePA = "test/shuffle" + task_name + "_" + env + "_PA.txt";
    std::string output_filePB = "test/shuffle" + task_name + "_" + env + "_PB.txt";
    PsiAnalyticsContext contextPA(PA, config_file, output_filePA);
    PsiAnalyticsContext contextPB(PB, config_file, output_filePB);
    // std::cout << "shuffle rows: " << argv[1] << std::endl;
    // PsiAnalyticsContext contextPA(PA,atoi(argv[1]),0);
    // PsiAnalyticsContext contextPB(PB,atoi(argv[1]),1);
    oc::Matrix<block> _b;
    // if(contextPA.fake_offline==true){
    _b.resize(contextPA.bins, 1);
    // }
    // oc::Matrix<block> _b(contextPA.bins, 2 * (contextPA.pa_features+contextPA.pb_features) + 1);
    for (int i = 0; i < _b.rows(); i++) {
        for (int j = 0; j < _b.cols(); j++) {
            _b(i, j) = block(i, j);
        }
    }
    // for (auto i = 0; i < _b.rows(); i++) {
    //     for (auto j = 0; j < _b.cols(); j++) {
    //         std::cout << _b(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "-----------------" << std::endl;
    auto p2 = std::async(std::launch::async, shuffle_receiver2, std::ref(contextPB));
    auto p1 = std::async(std::launch::async, shuffle_sender2, std::ref(_b), std::ref(contextPA));
    p1.get();
    p2.get();
    // share += _b;

    // contextPA.close();
    // contextPA.print();
    // contextPB.close();
    // contextPB.print();
    return 0;
    // p1.get();
    // auto [share, p] = p2.get();
    // share += _b;

    // std::cout << "-----------------" << std::endl;
    // for (auto i = 0; i < share.rows(); i++) {
    //     for (auto j = 0; j < share.cols(); j++) {
    //         std::cout << share(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }
    // std::cout << "-----------------" << std::endl;
    // for (auto pp : p) {
    //     std::cout << pp << " ";
    // }
    // std::cout << std::endl;
}