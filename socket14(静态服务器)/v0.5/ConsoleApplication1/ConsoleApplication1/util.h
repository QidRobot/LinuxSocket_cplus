#pragma once
#ifndef UTIL
#define UTIL
//c++�е�<cstdlib> �൱��c�����е�stdlib���
#include <cstdlib>
#include <string>
ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string &inBuffer);

ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &sbuff);

void handle_for_sigpipe();
//���÷�����
int setSockNonBlocking(int fd);
#endif
