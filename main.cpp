#include <cryptoTools/Common/Log.h>
#include <filesystem>
#include <libOTe/Tools/ExConvCode/ExConvCode.h>
#include <volePSI/Defines.h>
#include <volePSI/RsOpprf.h>
#include "client.h"
#include "context.h"
#include "server.h"

int main(int argc, char **argv)
{
    std::string config_file = argv[1];
    std::string env = argv[2];

    std::filesystem::path config_path(config_file);
    std::string task_name = config_path.stem().string();
    std::cout << osuCrypto::IoStream::lock;
    std::cout << config_file << std::endl;
    std::cout << osuCrypto::IoStream::unlock;
    std::string output_filePA = "test/two/" + task_name + "_" + env + "_PA.txt";
    std::string output_filePB = "test/two/" + task_name + "_" + env + "_PB.txt";
    PsiAnalyticsContext contextPA(PA, config_file, output_filePA);
    PsiAnalyticsContext contextPB(PB, config_file, output_filePB);
    auto futurePA = std::async(std::launch::async, client_run, std::ref(contextPA));
    auto futurePB = std::async(std::launch::async, server_run, std::ref(contextPB));
    std::vector<std::future<void>> futures;
    futures.push_back(std::move(futurePA));
    futures.push_back(std::move(futurePB));
    std::exception_ptr eptr = nullptr;
    while (!futures.empty()) {
        for (auto it = futures.begin(); it != futures.end();) {
            auto status = it->wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                try {
                    it->get(); // 获取结果或抛出异常
                } catch (...) {
                    eptr = std::current_exception(); // 捕获异常
                }
                it = futures.erase(it); // 移除已完成的future
            } else {
                ++it;
            }
        }
        if (eptr) {
            break;
        }
    }
    if (eptr) {
        try {
            std::rethrow_exception(eptr); // 重新抛出异常
        } catch (const std::exception &e) {
            std::cerr << "Exception caught: " << e.what() << std::endl;
        }
        std::terminate(); // 终止程序以避免进一步的死锁或其他问题
    }
    return 0;
}