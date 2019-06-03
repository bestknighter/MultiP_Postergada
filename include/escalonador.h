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

struct sembuf operacao[2];

int p_sem();
int v_sem();

char * buscar_info(int *tempoEspera, int *jobID);
double esperar_mensagens();
double tempo_execucao(char *msg_termino);
void enviar_mensagem_postergado(int *jobID);

#endif
