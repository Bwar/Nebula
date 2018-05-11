/*
 * process_helper.c
 *
 *  Created on: 2014骞�7鏈�26鏃�
 *      Author: lbh
 */

#include <string.h>
#include "process_helper.h"


#ifdef __cplusplus
extern "C" {
#endif

void InstallSignal()
{
    signal(SIGHUP  ,SIG_IGN );   /* hangup, generated when terminal disconnects */
    signal(SIGINT  ,SIG_IGN );   /* interrupt, generated from terminal special char */
    signal(SIGQUIT ,SIG_IGN );   /* (*) quit, generated from terminal special char */
    signal(SIGILL  ,SIG_IGN );   /* (*) illegal instruction (not reset when caught)*/
    signal(SIGTRAP ,SIG_IGN );   /* (*) trace trap (not reset when caught) */
    signal(SIGABRT ,SIG_IGN );   /* (*) abort process */
#ifdef D_AIX
    signal(SIGEMT  ,SIG_IGN );   /* EMT intruction */
#endif
    signal(SIGFPE  ,SIG_IGN );   /* (*) floating point exception */
    signal(SIGKILL ,SIG_IGN );   /* kill (cannot be caught or ignored) */
    signal(SIGBUS  ,SIG_IGN );   /* (*) bus error (specification exception) */
    signal(SIGSEGV ,SIG_IGN );   /* (*) segmentation violation */
    signal(SIGSYS  ,SIG_IGN );   /* (*) bad argument to system call */
    signal(SIGPIPE ,SIG_IGN );   /* write on a pipe with no one to read it */
    signal(SIGALRM ,SIG_IGN );   /* alarm clock timeout */
    //signal(SIGTERM ,stopproc );  /* software termination signal */
    signal(SIGURG  ,SIG_IGN );   /* (+) urgent contition on I/O channel */
    signal(SIGSTOP ,SIG_IGN );   /* (@) stop (cannot be caught or ignored) */
    signal(SIGTSTP ,SIG_IGN );   /* (@) interactive stop */
    signal(SIGCONT ,SIG_IGN );   /* (!) continue (cannot be caught or ignored) */
    //signal(SIGCHLD ,SIG_IGN);    /* (+) sent to parent on child stop or exit */
    signal(SIGTTIN ,SIG_IGN);    /* (@) background read attempted from control terminal*/
    signal(SIGTTOU ,SIG_IGN);    /* (@) background write attempted to control terminal */
    signal(SIGIO   ,SIG_IGN);    /* (+) I/O possible, or completed */
    signal(SIGXCPU ,SIG_IGN);    /* cpu time limit exceeded (see setrlimit()) */
    signal(SIGXFSZ ,SIG_IGN);    /* file size limit exceeded (see setrlimit()) */

#ifdef D_AIX
    signal(SIGMSG  ,SIG_IGN);    /* input data is in the ring buffer */
#endif

    signal(SIGWINCH,SIG_IGN);    /* (+) window size changed */
    signal(SIGPWR  ,SIG_IGN);    /* (+) power-fail restart */
    signal(SIGUSR1 ,SIG_IGN);   /* user defined signal 1 */
    signal(SIGUSR2 ,SIG_IGN);   /* user defined signal 2 */
    signal(SIGPROF ,SIG_IGN);    /* profiling time alarm (see setitimer) */

#ifdef D_AIX
    signal(SIGDANGER,SIG_IGN);   /* system crash imminent; free up some page space */
#endif

    signal(SIGVTALRM,SIG_IGN);   /* virtual time alarm (see setitimer) */

#ifdef D_AIX
    signal(SIGMIGRATE,SIG_IGN);  /* migrate process */
    signal(SIGPRE  ,SIG_IGN);    /* programming exception */
    signal(SIGVIRT ,SIG_IGN);    /* AIX virtual time alarm */
    signal(SIGALRM1,SIG_IGN);    /* m:n condition variables - RESERVED - DON'T USE */
    signal(SIGWAITING,SIG_IGN);  /* m:n scheduling - RESERVED - DON'T USE */
    signal(SIGCPUFAIL ,SIG_IGN); /* Predictive De-configuration of Processors - */
    signal(SIGKAP,SIG_IGN);      /* keep alive poll from native keyboard */
    signal(SIGRETRACT,SIG_IGN);  /* monitor mode should be relinguished */
    signal(SIGSOUND  ,SIG_IGN);  /* sound control has completed */
    signal(SIGSAK    ,SIG_IGN);  /* secure attention key */
#endif
}

void daemonize(const char* cmd)
{
//    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
    {
//        m_pLog->WriteLog(ERROR, "%s: error %d can't get file limit: %s", cmd,
//                errno, strerror_r(errno, m_szErrBuffer, loss::gc_uiMaxErrBufferLength));
    }

    // fork锛岀粓姝㈢埗杩涚▼
    if((pid = fork()) < 0)
    {
//        m_pLog->WriteLog(ERROR, "%s error %d can't fork: %s", cmd,
//                errno, strerror_r(errno, m_szErrBuffer, loss::gc_uiMaxErrBufferLength));
    }
    else if (pid != 0)
    {
        exit(0);
    }

    // 璁剧疆绗竴瀛愯繘绋�
    setsid();

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
//        m_pLog->WriteLog(ERROR, "%s error %d can't ignore SIGHUP: %s", cmd,
//                errno, strerror_r(errno, m_szErrBuffer, loss::gc_uiMaxErrBufferLength));
    }

    // fork锛岀粓姝㈢涓�瀛愯繘绋�
    if ((pid = fork()) < 0)
    {
//        m_pLog->WriteLog(ERROR, "%s error %d can't fork: %s", cmd,
//                errno, strerror_r(errno, m_szErrBuffer, loss::gc_uiMaxErrBufferLength));
    }
    else if (pid != 0)
    {
        exit(0);
    }

    InstallSignal();

//    // 灏嗗伐浣滅洰褰曡缃负鈥�/鈥�
//    if (chdir("/") < 0)
//    {
//        m_pLog->WriteLog(ERROR, "%s error %d can't change directory to /: %s",
//                errno, strerror_r(errno, m_szErrBuffer, loss::gc_uiMaxErrBufferLength));
//    }

    // 娓呴櫎鏂囦欢鎺╃爜
    umask(0);

//    // 鍏抽棴鎵�鏈夋枃浠跺彞鏌�
//    for (i=0; i<MAXFD; i++)
//    {
//    	close(i);
//    }

    if (rl.rlim_max == RLIM_INFINITY)
    {
        rl.rlim_max = 1024;
    }
    /*
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "unexpected file descriptors %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
    */
}

int x_sock_set_block(int sock, int on)
{
        int             val;
        int             rv;

        val = fcntl(sock, F_GETFL, 0);
        if (on) {
                rv = fcntl(sock, F_SETFL, ~O_NONBLOCK&val);
        } else {
                rv = fcntl(sock, F_SETFL, O_NONBLOCK|val);
        }

        if (rv) {
                return (int)(errno);
        }

        return 0;
}

int readable_timeo(int fd, int sec)
{
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return select(fd+1, &rset, NULL, NULL, &tv);
}

#ifdef __cplusplus
}
#endif

