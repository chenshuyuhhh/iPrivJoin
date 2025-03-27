#pragma once
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/Socket.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Matrix.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include "toml.hpp"
#include "network.h"

using block = osuCrypto::block;

enum Role { PA, PB };

struct PsiAnalyticsContext {
    Role role;
    std::string address;
    uint64_t port;
    uint64_t bins;
    uint64_t funcs;

    uint64_t pa_elems;
    uint64_t pa_features;
    coproto::Socket chl;
    uint64_t pb_elems;
    uint64_t pb_features;
    int socket_fd;

    std::string output_file;

    uint64_t layers;
    uint64_t seedJ;
    std::vector<block> J;
    uint64_t threads;

    bool is_test;
    bool use_ture_shuflle;
    bool setuped = false;
    bool fake_offline = false;
    osuCrypto::Timer *timer;
    unsigned long totalSend = 0;
    unsigned long totalReceive = 0;

    PsiAnalyticsContext(Role role, std::string config_file, std::string output_file = "")
        : role(role), output_file(output_file)
    {
        auto table = toml::parse_file(config_file);

        funcs = table["funcs"].value_or(2);
        address = table["address"].value_or("localhost:10011");
        port = table["port"].value_or(9011);

        pa_elems = table["pa_elems"].value_or(0);
        pb_elems = table["pb_elems"].value_or(0);

        pa_features = table["pa_features"].value_or(0);
        pb_features = table["pb_features"].value_or(0);

        // even for simplicity
        bins =
            table["bins"].value_or(funcs == 2 ? int(2.4 * pa_elems) : int(1.27 * pa_elems)) / 2 * 2;
        fake_offline = table["fake_offline"].value_or(false);
        layers = table["layers"].value_or(2 * osuCrypto::log2ceil(bins) - 1);
        seedJ = table["seed"].value_or(99526);
        is_test = table["test"].value_or(false);
        use_ture_shuflle = table["use_shuflle"].value_or(false);
        threads = table["threads"].value_or(1);
    }

    PsiAnalyticsContext(Role role, int bins, int f) : role(role), bins(bins)
    {
        is_test = true;
        address = "localhost:10011";
        port = 1011;
        layers = 2 * osuCrypto::log2ceil(bins) - 1;
        pa_features = f;
        pb_features = f;
        osuCrypto::PRNG prng((block(10)));
        J = std::vector<block>(1 + pa_features + pb_features);
        prng.get(J.data(), 1 + pa_features + pb_features);
    }

    void print()
    {
        std::fstream fs(output_file, std::ios::out | std::ios::app);
        std::ostream &out = fs.is_open() ? fs : std::cout;
        out << "\n";
        out << *timer;
        out << "Receive: " << totalReceive * 1.0 / 1048576 << "MB\n";
        out << "Send:    " << totalSend * 1.0 / 1048576 << "MB\n";
        out << "Total:   " << (totalReceive + totalSend) * 1.0 / 1048576 << "MB\n";
        out << "\n";
        if (fs.is_open()) {
            fs.close();
        }
        return;
    }

    void setup()
    {
        osuCrypto::PRNG prng((block(seedJ)));
        J = std::vector<block>(1 + pa_features + pb_features);
        prng.get(J.data(), 1 + pa_features + pb_features);

        if (role == PA) {
            chl = coproto::asioConnect(address, true);
        } else {
            chl = coproto::asioConnect(address, false);
        }

        socket_fd = -1;
        if(role== PA){
            std::string ip = "0.0.0.0";
            connect_others(1, ip.data(), port, socket_fd);
        }else{
            wait_for_connect(0, port, socket_fd);
        }
        // greet(int socket_fd)
        if (socket_fd >= 0) {
            setuped = true;
            std::cout << "connect success" << std::endl;
        }
    }

    void close()
    {
        totalReceive = chl.bytesReceived() + get_comm_size_recv();
        totalSend = chl.bytesSent() + get_comm_size_send();

        coproto::sync_wait(chl.close());
        close_socket(socket_fd);
    }

    void setTimer(osuCrypto::Timer &other)
    {
        this->timer = &other;
    }
};
