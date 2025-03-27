#include <cryptoTools/Common/Timer.h>
#include <cstdlib>
#include "context.h"
#include "ggm.h"
int main(int argc, char *argv[])
{
    osuCrypto::Timer timer;
    timer.reset();
    timer.setTimePoint("start");
    GGMTree ggm(std::atoi(argv[1]), block(1999, 526));
    // ggm.print();
    timer.setTimePoint("end");
    std::cout << timer << std::endl;
    return 0;
}