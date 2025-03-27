#include <coproto/Socket/LocalAsyncSock.h>
#include <cryptoTools/Common/Log.h>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <shuffle.h>
#include <utlis.h>
#include <vector>
#include <volePSI/Defines.h>
#include "context.h"

int main(int argc, char *argv[])
{
    std::string config_file = argv[1];
    std::string env = argv[2];

    std::filesystem::path config_path(config_file);
    std::string task_name = config_path.stem().string();
    std::cout << osuCrypto::IoStream::lock;
    std::cout << config_file << std::endl;
    std::cout << osuCrypto::IoStream::unlock;
    std::string output_filePA = "test/shu" + task_name + "_" + env + "_PA.txt";
    std::string output_filePB = "test/shu" + task_name + "_" + env + "_PB.txt";
    PsiAnalyticsContext contextPA(PA, config_file, output_filePA);
    PsiAnalyticsContext contextPB(PB, config_file, output_filePB);

    // std::cout << "-----------------" << std::endl;
    auto p2 = std::async(std::launch::async, shuffle_receivermul, std::ref(contextPB));
    auto p1 = std::async(std::launch::async, shuffle_sendermul, std::ref(contextPA));
    p1.get();
    p2.get();
    return 0;
}