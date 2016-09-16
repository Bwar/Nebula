/*
 * process_helper.h
 *
 *  Created on: 2014骞�7鏈�26鏃�
 *      Author: lbh
 */

#ifndef PROCESS_HELPER_H_
#define PROCESS_HELPER_H_

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <sys/resource.h>

#define BUF_SIZE 1024

#ifdef __cplusplus
extern "C" {
#endif


//int send_fd(int fd, int fd_to_send);
//int recv_fd(int fd, char *buffer, size_t size);
//int daemonize();
void InstallSignal();
void daemonize(const char* cmd);
int x_sock_set_block(int sock, int on);
int send_fd(int sock_fd, int send_fd) ;
int recv_fd(const int sock_fd);
int send_fd_with_attr(int sock_fd, int send_fd, void* addr, int addr_len, int send_fd_attr);
int recv_fd_with_attr(const int sock_fd, void* addr, int addr_len, int* send_fd_attr);
int readable_timeo(int fd, int sec);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_HELPER_H_ */
