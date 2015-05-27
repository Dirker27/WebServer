#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

void init_signals(void);
void signal_handler(int);

struct sigaction sigact;

void init_signals() {
	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	//sigaction(SIGINT, &sigact, (struct sigaction*) NULL);
	sigaction(SIGPIPE, &sigact, (struct sigaction*) NULL);
	//sigaction(SIGTSTP, &sigact, (struct sigaction*) NULL);
	//sigaction(SIGCHLD, &sigact, (struct sigaction*) NULL);
}

void signal_handler(int sig) {
	switch(sig) {
		case SIGINT:
			g_shutdown = true;
			break;

		case SIGTSTP:
			break;

		case SIGPIPE:
			/*if (errno == EPIPE) {
				printf("EPIPE Error!\n");
			}*/
			break;

		default:
			break;
	}
}

#endif
