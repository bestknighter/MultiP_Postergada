/* Semaforo implementado por Alba Melo. */
#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>

// Declare the message structure.
#define MSGSZ 128

typedef struct msgbuf {
	long mtype;
	char mtext[MSGSZ];
} message_buf;

message_buf rbuf;

struct sembuf operacao[2];
int idsem;

void executa_programa(message_buf * msg_postergada);

int p_sem();
int v_sem();

#endif