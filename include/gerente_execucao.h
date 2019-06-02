#ifndef GERENTE_EXECUCAO_H
#define GERENTE_EXECUCAO_H

#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define HYPERCUBE 0
#define TORUS 1
#define FAT_TREE 2

typedef struct {
	int id;
	pid_t pid;
	key_t msqKey;
	int msqID;
	int busy;
} gerente_metadados_t;

typedef struct {
	gerente_metadados_t self;
	gerente_metadados_t escalonador;
	int nVizinhos;
	gerente_metadados_t *noVizinho;
} gerente_init_t;

gerente_init_t* cria_gerentes( int topologia );

#endif // GERENTE_EXECUCAO_H
