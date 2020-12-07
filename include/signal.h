/* include/signal.h */

#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef unsigned int sigset_t;

#define SIGCHLD 17  // child process ends

#define SA_NOMASK  0x40000000  // do not mask the signal when the signal is being handled
#define SA_ONESHOT  0x80000000  // one-time shot, only called once - restoring to the default handler once the handler is called

#define SIG_DFL ((void (*)(int))0)  // default signal handling
#define SIG_IGN ((void (*)(int))1)  // ignore signal

struct sigaction {
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)();
};

#endif
