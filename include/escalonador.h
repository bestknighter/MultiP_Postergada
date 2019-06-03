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
#include <time.h>

#include "../include/gerente_execucao.h"

// Declare the message structure.
#define MSGSZ 128

int topologia;

typedef struct msgbuf {
  long mtype;
  char mtext[MSGSZ];
} message_buf;

message_buf rbuf;

typedef struct msgTerminoGerentes {
  long mtype;
  char mtext[64];
} msg_termino_gerentes;

msg_termino_gerentes msg_termino;

struct sembuf operacao[2];
int idsem;

void executa_programa();

int p_sem();
int v_sem();

char * buscar_info(int *tempoEspera, int *jobID, char *nomePrograma);
double esperar_mensagens();
double tempo_execucao(char *msg_termino);
void enviar_mensagem_postergado(int *jobID);

#endif
