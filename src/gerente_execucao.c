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

void gerente_loop( gerente_init_t* dadosIniciais, int myID) {
	dados = dadosIniciais[myID];
	free( dadosIniciais );
	dados.self.pid = getpid();

	printf( "Gerente %d iniciando execucao... Meus vizinhos sao %d, %d, %d e %d.\n", dados.noVizinho[0].id, dados.noVizinho[1].id, dados.noVizinho[2].id, dados.noVizinho[3].id );
	
	while( 1 ) {
		// verifica se tem msgs pra receber, se sim
			// se tiver que encaminhar alguma msg, faça
			// se tiver de executar algum programa
				// exec();

		int wstatus;
		if( 0 != waitpid( -1, &wstatus, WNOHANG ) ) {
			end_exec();
		}
	}
}

gerente_init_t* cria_gerentes( int topologia ) {
	if( topologia < HYPERCUBE || FAT_TREE < topologia ) {
		fprintf( stderr, "[ERRO] Topologia desconhecida!\nExistentes: 0 - Hypercubo, 1 - Torus, 2 - Fat Tree.\nFornecida: %d\n\n", topologia );
		return NULL;
	}

	gerente_init_t* gerentes;

	gerentes = (gerente_init_t*) calloc(16, sizeof(gerente_init_t));

	gerentes[0].escalonador.pid = getpid();
	gerentes[0].escalonador.msqKey = escalonadorMsqKey;
	gerentes[0].escalonador.msqID = escalonadorMsqID;

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
		// Bit 3 = Cubo 0 ou Cubo 1 (Eixo W)
		// Bit 2 = Face superior ou Face inferior (Eixo Z)
		// Bit 1 = Face esquerda ou Face direita (Eixo Y)
		// Bit 0 = Face próxima ou Face distante (Eixo X)
		// i.e.:	0000 = Canto próximo esquerdo superior do cubo 0
		// 			1010 = Canto próximo direito superior do cubo 1
		for( int i = 0; i < 16; i++ ) {
			gerentes[i].noVizinho[0] = gerentes[ i^0b0001 ].self;		// FLIP X
			gerentes[i].noVizinho[1] = gerentes[ i^0b0010 ].self;		// FLIP Y
			gerentes[i].noVizinho[2] = gerentes[ i^0b0100 ].self;		// FLIP Z
			gerentes[i].noVizinho[3] = gerentes[ i^0b1000 ].self;		// FLIP W
		}
	} else if( TORUS == topologia ) {
		for( int i = 0; i < 16; i++ ) {
			gerentes[i].noVizinho[0] = gerentes[ (i+12)%16 ].self;						// UP
			gerentes[i].noVizinho[1] = gerentes[ ((i%4)+1)%4 + 4*((int)i/4) ].self;		// RIGHT
			gerentes[i].noVizinho[2] = gerentes[ (i+4)%16 ].self;						// DOWN
			gerentes[i].noVizinho[3] = gerentes[ ((i%4)+3)%4 + 4*((int)i/4) ].self;		// LEFT
		}
	} else { // Fat Tree
		// Como conectar um Fat Tree??
	}

	for( int i = 0; i < 16; i++ ) {
		if( FAT_TREE == topologia && 15 == i ) break;

		int pid;
		if( 0 == (pid = fork()) ) {
			gerente_loop( gerentes, i );
		} else {
			gerentes[i].self.pid = pid;
		}
	}

	return gerentes;
}
