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



//int daemonize();
void InstallSignal();
void daemonize(const char* cmd);
int x_sock_set_block(int sock, int on);
int readable_timeo(int fd, int sec);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_HELPER_H_ */
