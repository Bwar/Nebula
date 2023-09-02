/*
 * Copyright (C) Igor Sysoev
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "proctitle_helper.h"

#if defined(__linux__)
# if !defined(HAVE_PR_SET_NAME)
#  include <linux/version.h>
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
#include <sys/prctl.h>
#define HAVE_PR_SET_NAME 1
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,9)
#else
#endif // !defined(HAVE_PR_SET_NAME)
#endif // defined(linux)
/*
 * To change the process title in Linux and Solaris we have to set argv[1]
 * to NULL and to copy the title to the same place where the argv[0] points to.
 * However, argv[0] may be too small to hold a new title.  Fortunately, Linux
 * and Solaris store argv[] and environ[] one after another.  So we should
 * ensure that is the continuous memory and then we allocate the new memory
 * for environ[] and copy it.  After this we could use the memory starting
 * from argv[0] for our process title.
 *
 * The Solaris's standard /bin/ps does not show the changed process title.
 * You have to use "/usr/ucb/ps -w" instead.  Besides, the UCB ps dos not
 * show a new title if its length less than the origin command line length.
 * To avoid it we append to a new title the origin command line in the
 * parenthesis.
 */
extern char **environ;

static char *ngx_os_argv_last;

static int ngx_argc;
static int total_arg_buf_size;
static char **ngx_argv;
static char **ngx_os_argv;

static int ngx_setproctitle_init = 0;

#define NGX_SETPROCTITLE_USES_ENV  1
#define NGX_SETPROCTITLE_PAD       '\0'

int ngx_init_setproctitle(int argc, char** argv)
{
	ngx_os_argv = argv;
	ngx_argv = argv;
	ngx_argc = argc;

	char *p;
	size_t size;
	int i;

	size = 0;
	total_arg_buf_size = 0;
	for (i = 0; environ[i]; i++)
	{
		size += strlen(environ[i]) + 1;
	}
	total_arg_buf_size = size;

	p = (char*) malloc(size);
	if (p == NULL)
	{
		return -1;
	}

	ngx_os_argv_last = ngx_os_argv[0];

	for (i = 0; ngx_os_argv[i]; i++)
	{
		if (ngx_os_argv_last == ngx_os_argv[i])
		{
			ngx_os_argv_last = ngx_os_argv[i] + strlen(ngx_os_argv[i]) + 1;
		}
		total_arg_buf_size += (strlen(ngx_os_argv[i]) + 1);
	}
	total_arg_buf_size--;

	for (i = 0; environ[i]; i++)
	{
		if (ngx_os_argv_last == environ[i])
		{

			size = strlen(environ[i]) + 1;
			ngx_os_argv_last = environ[i] + size;
			//strncpy(p, (char *) environ[i], size);
			snprintf(p, size, "%s", environ[i]);
			environ[i] = (char *) p;
			p += size;
		}
	}

	ngx_os_argv_last--;
	ngx_setproctitle_init = 1;
	return 0;
}

void ngx_setproctitle(const char *title)
{
	if(ngx_setproctitle_init != 1)
	{
		return;
	}
	//ASSERT(ngx_setproctitle_init == 1 && NULL != title);
	//char *p;

	ngx_os_argv[1] = NULL;


	//p = strncpy((char *) ngx_os_argv[0], title, ngx_os_argv_last
	//        - ngx_os_argv[0]);
	int new_title_len = strlen(title);
	if (new_title_len < total_arg_buf_size)
	{
	    memset(ngx_os_argv[0], 0, total_arg_buf_size);
		sprintf(ngx_os_argv[0], "%s", title);
	}
	else
	{
		memcpy(ngx_os_argv[0], title, total_arg_buf_size);
		ngx_os_argv[0][total_arg_buf_size] = 0;
	}


	//if (ngx_os_argv_last - (char *) p)
	{
		//memset(p, NGX_SETPROCTITLE_PAD, ngx_os_argv_last - (char *) p);
	}
	/*
	 * Try change process name
	 */
#ifdef HAVE_PR_SET_NAME
	prctl(PR_SET_NAME,title,0,0,0);
#endif
}

const char* getproctitle()
{
    static char proctitle[2048];
	char linkname[64]; /* /proc/<pid>/exe */
	pid_t pid;


	/* Get our PID and build the name of the link in /proc */
	pid = getpid();

	if (snprintf(linkname, sizeof(linkname), "/proc/%i/cmdline", pid) < 0)
	{
		/* This should only happen on large word systems. I'm not sure
		 what the proper response is here.
		 Since it really is an assert-like condition, aborting the
		 program seems to be in order. */
		abort();
	}

	/* Now read the file */
	//ret = readlink(linkname, proctitle, sizeof(proctitle) - 1);
	int cmdline_fd = open(linkname, O_RDONLY);
	if(cmdline_fd < 0)
	{
		return NULL;
	}

	size_t ret = read(cmdline_fd, proctitle, sizeof(proctitle)-1);


	/* Report insufficient buffer size */
	if (ret == (sizeof(proctitle)-1))
	{
		errno = ERANGE;
		return NULL;
	}

	/* Ensure proper NUL termination */
	proctitle[ret] = 0;
	close(cmdline_fd);
	return proctitle;
}

