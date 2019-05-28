#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gerente_execucao.h"

extern key_t escalonadorMsqKey;
extern int escalonadorMsqID;

time_t start, end;
pid_t proc;
gerente_init_t dados;

void exec() {
	if( 0 == (proc = fork()) ) {
		// execl();
	} else {
		start = time(NULL);
	}
}

void end_exec() {
	end = time(NULL);
	// avisa escalonador que terminou com tempo start e end
}

gerente_init_t* cria_gerentes( int topologia ) {
	if( HYPERCUBE > topologia || FAT_TREE < topologia ) {
		fprintf( stderr, "[ERRO] Topologia desconhecida!\nExistentes: 0 - Hypercubo, 1 - Torus, 2 - Fat Tree.\nFornecida: %d\n\n", topologia );
		return NULL;
	}

	gerente_init_t* gerentes;

	gerentes = (gerente_init_t*) calloc(16, sizeof(gerente_init_t));

	gerentes[0]->escalonador.pid = getpid();
	gerentes[0]->escalonador.msqKey = escalonadorMsqKey;
	gerentes[0]->escalonador.msqID = escalonadorMsqID;

	for( int i = 0; i < 16; i++ ) {
		gerentes[i].self.id = i;
		gerentes[i].self.msqKey = 0x1233 + i;
		gerentes[i].self.msqID = msgget( gerentes[i].self.msqKey, IPC_CREAT|0x1ff );
		if( 0 > gerentes[i].self.msqID ) {
			fprintf( stderr, "[ERRO] Erro na obtencao de fila para o gerente %d.\n\n", i );
			free( gerentes );
			return NULL;
		}
	}

	if( HYPERCUBE == topologia ) {
		// Como conectar um hypercube??
	} else if( TORUS == topologia ) {
		for( int i = 0; i < 16; i++ ) {
			gerentes[i].noVizinho[0] = gerentes[ (i+12)%16 ].self; // UP
			gerentes[i].noVizinho[1] = gerentes[ ((i%4)+1)%4 + 4*((int)i/4) ].self; // RIGHT
			gerentes[i].noVizinho[2] = gerentes[ (i+4)%16 ].self; // DOWN
			gerentes[i].noVizinho[3] = gerentes[ ((i%4)+3)%4 + 4*((int)i/4) ].self; // LEFT
		}
	} else { // Fat Tree
		// Como conectar um Fat Tree??
	}

	return gerentes;
}

void gerente_loop( gerente_init_t dadosIniciais) {
	dados = dadosIniciais;
	dados.self.pid = getpid();
	while( 1 ) {
		// verifica se tem msgs pra receber, se sim
			// se tiver que encaminhar alguma msg, faça
			// se tiver de executar algum programa
				// exec();

		// verifica se terminou de executar, se sim
			// end_exec();
	}
}
