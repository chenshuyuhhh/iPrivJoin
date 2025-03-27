#include "network.h"

#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <mutex>
#include <iostream>
#include "thread"


// std::mutex communication_size_send_mtx;
// std::mutex communication_size_recv_mtx;
uint64_t communication_size_send = 0;
uint64_t communication_size_recv = 0;
const int socket_buffer_size = 33554432;
// int socket_fd;

void connect_others(int i, const char *ip, int port, int &socket_fd) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    if (::bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind");
        close(sock);
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return;
    }
    serv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sock);
        return;
    }

    socket_fd = sock;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&socket_buffer_size, sizeof(int)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        return;
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (const char *)&socket_buffer_size, sizeof(int)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        return;
    }
}

void wait_for_connect(int i, int port, int &socket_fd) {
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock < 0) {
        perror("socket");
        return;
    }

    int opt = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(serv_sock);
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    serv_addr.sin_port = htons(port);

    if (::bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        close(serv_sock);
        return;
    }

    if (listen(serv_sock, 20) < 0) {
        perror("listen");
        close(serv_sock);
        return;
    }

    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    socket_fd = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (socket_fd < 0) {
        perror("accept");
        close(serv_sock);
        return;
    }

    close(serv_sock);

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, (const char *)&socket_buffer_size, sizeof(int)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        return;
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (const char *)&socket_buffer_size, sizeof(int)) < 0) {
        perror("setsockopt");
        close(socket_fd);
        return;
    }
}

    // void greet(int nParties, int myIdx, int *socket_fds)
    // {
    //     int *temp_pid = new int[nParties];
    //     temp_pid[myIdx] = 0;
    //     for (int i = 0; i < nParties; i++)
    //     {
    //         if (socket_fds[i] != 0)
    //             send_n(socket_fds[i], (char *)&myIdx, 4);
    //     }
    //     for (int i = 0; i < nParties; i++)
    //     {
    //         int id;
    //         if (socket_fds[i] != 0)
    //         {
    //             recv_n(socket_fds[i], (char *)&id, 4);
    //             // #ifdef DEGUG_INFO
    //             // std::cout << "id " << id << std::endl;
    //             // #endif
    //             temp_pid[id] = socket_fds[i];
    //         }
    //     }
    //     for (int i = 0; i < nParties; i++)
    //         socket_fds[i] = temp_pid[i];
    //     delete[] temp_pid;
    // }

void send_n(int fd, char *data, size_t len)
{
    uint64_t temp = 0;
    while (temp < len)
    {
        temp = temp + write(fd, data + temp, len - temp);
    }

    // communication_size_send_mtx.lock();
    communication_size_send = communication_size_send + len;
    // communication_size_send_mtx.unlock();
}

void recv_n(int fd, char *data, size_t len)
{
    uint64_t temp = 0;
    while (temp < len)
    {
        temp = temp + read(fd, data + temp, len - temp);
    }

    // communication_size_recv_mtx.lock();
    communication_size_recv = communication_size_recv + len;
    // communication_size_recv_mtx.unlock();
}

void send_array(const Mat2d &mat, int socket_fd)
{
    send_n(socket_fd, (char *)mat.data(), mat.size() * sizeof(ringsize));
}

void recv_array(int socket_fd, Mat2d &mat)
{
    // int r = mat.rows();
    // int c = mat.cols();
    // send_n(socket_fds[pid], (char *)&r, 4);
    // send_n(socket_fds[pid], (char *)&c, 4);
    // std::cout << mat.size()  << " " << mat.size() * sizeof(ringsize) << std::endl;
    recv_n(socket_fd, (char *)mat.data(), mat.size() * sizeof(ringsize));
}

uint64_t get_comm_size_send()
{
    return communication_size_send;
}

uint64_t get_comm_size_recv()
{
    return communication_size_recv;
}

void close_socket(int socket_fd)
{
    close(socket_fd);
    socket_fd = 0;
}