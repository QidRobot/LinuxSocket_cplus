#ifndef UTIL
#define UTIL
//c++�е�<cstdlib> �൱��c�����е�stdlib���
#include <cstdlib>
ssize_t readn(int fd, void *buff, size_t n);
ssize_t writen(int fd, void *buff, size_t n);
void handle_for_sigpipe();
//���÷�����
int setSockNonBlocking(int fd);
#endif
