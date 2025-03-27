#include <Eigen/Dense>

typedef uint64_t ringsize;
typedef Eigen::Matrix<uint64_t, -1, -1> Mat2d;
// int socket_fd;

void connect_others(int i, const char *ip, int port, int &socket_fd);
void wait_for_connect(int i, int port, int &socket_fd);
// void greet(int socket_fd);

void send_n(int fd, char *data, int len);
void recv_n(int fd, char *data, int len);
void send_array(const Mat2d &mat, int socket_fd);
void recv_array(int socket_fd, Mat2d &mat);

uint64_t get_comm_size_send();
uint64_t get_comm_size_recv();
void close_socket(int socket_fd);
