
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SETPROCTITLE_H_INCLUDED_
#define _NGX_SETPROCTITLE_H_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * setproctitle Port from nginx
 */

int ngx_init_setproctitle(int argc, char** argv);
void ngx_setproctitle(const char *title);

const char* getproctitle();


#ifdef __cplusplus
}
#endif

#endif /* _NGX_SETPROCTITLE_H_INCLUDED_ */
