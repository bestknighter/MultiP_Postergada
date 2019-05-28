#ifndef GERENTE_EXECUCAO_H
#define GERENTE_EXECUCAO_H

#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define HYPERCUBE 0
#define TORUS 1
#define FAT_TREE 2

typedef struct {
	int id;
	pid_t pid;
	key_t msqKey;
	int msqID;
} gerente_metadados_t;

typedef struct {
	gerente_metadados_t self;
	gerente_metadados_t escalonador;
	gerente_metadados_t noVizinho[4];
} gerente_init_t;

gerente_init_t* cria_gerentes( int topologia );

#endif // GERENTE_EXECUCAO_H
