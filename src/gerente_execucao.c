#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#include "../include/gerente_execucao.h"

#define MSG_START 10
#define MSG_END 11

extern key_t escalonadorMsqKey;
extern int escalonadorMsqID;

typedef struct {
	int jobID;
	pid_t procPID;
	char* program;
	time_t start;
	time_t end;
} job_t;

struct msgBuf {
	long mtype;
	char mtext[64];
} msg;
gerente_init_t dados;
job_t jobAtual;

int stringToJob( char* msg, job_t* job ) {
	int IDSender;
	sscanf( msg, "%d %s %d", &(job->jobID), &(job->program), &IDSender );
	return IDSender;
}

void sendJob( int msqID, job_t job ) {
	struct msgBuf newMsg;
	newMsg.mtype = MSG_START;
	sprintf( newMsg.mtext, "%d %s %d", job.jobID, job.program, dados.self.id );
	msgsnd( msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
}

void rcvJob( char* mtext ) {
	job_t jobRecebido;
	int IDSender = stringToJob( mtext, &jobRecebido );
	if( jobRecebido.jobID != jobAtual.jobID ) {
		for(int i = 0; i < dados.self.nVizinhos; i++ ) {
			if( dados.noVizinho[i].id == IDSender ) continue;
			sendJob( dados.noVizinho[i].msqID, jobRecebido );
		}
		jobAtual = jobRecebido;
		exec();
	}
}

void exec() {
	if( 0 == (jobAtual.procPID = fork()) ) {
		execl( jobAtual.program, jobAtual.program, (char*) NULL );
	} else {
		jobAtual.start = time(NULL);
		dados.self.busy = 1;
	}
}

void end_exec() {
	jobAtual.end = time(NULL);
	struct msgBuf newMsg;
	newMsg.mtype = MSG_END;
	sprintf( newMsg.mtext, "%d %d %d %d", job.jobID, dados.self.id, (int)jobAtual.start, (int)jobAtual.end );
	if( dados.self.id == 0 ) {
		msgsnd( dados.escalonador.msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
	} else {
		int vizinhoLowestID;
		int lowestID = 99;
		for( int i = 0; i < dados.self.nVizinhos; i++ ) {
			if( dados.noVizinho[i].id < lowestID ) vizinhoLowestID = i;
		}
		msgsnd( dados.noVizinho[vizinhoLowestID].msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
	}
	dados.self.busy = 0;
}

void gerente_loop( gerente_init_t* dadosIniciais, int myID) {
	dados = dadosIniciais[myID];
	free( dadosIniciais );
	dados.self.pid = getpid();
	jobAtual.jobID = -1;
	jobAtual.procPID = 0;

	printf( "Gerente %d iniciando execucao... Meus vizinhos sao", dados.self.id );
	for(int i = 0; i < dados.nVizinhos; i++ ) {
		printf( " %d", dados.noVizinho[i].id );
	}
	printf( "\n" );
	
	while( 1 ) {
		if( -1 < msgrcv( dados.self.msqID, &msg, 64, 0, IPC_NOWAIT ) ) {
			if( MSG_START == msg.mtype ) {
				rcvJob( msg.mtext );
			} else if( MSG_END == msg.mtext ) {
				if( dados.self.id == 0 ) {
					msgsnd( dados.escalonador.msqID, &msg, sizeof(msg.mtext), IPC_NOWAIT );
				} else {
					int vizinhoLowestID;
					int lowestID = 99;
					for( int i = 0; i < dados.self.nVizinhos; i++ ) {
						if( dados.noVizinho[i].id < lowestID ) vizinhoLowestID = i;
					}
					msgsnd( dados.noVizinho[vizinhoLowestID].msqID, &msg, sizeof(msg.mtext), IPC_NOWAIT );
				}
			}
		}

		int wstatus;
		if( 0 < waitpid( -1, &wstatus, WNOHANG ) ) {
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
		// 			1111 = Canto distante direito inferior do cubo 1
		for( int i = 0; i < 16; i++ ) {
			gerentes[i].nVizinhos = 4;
			gerentes[i].noVizinho = calloc( 4, sizeof(gerente_metadados_t) );
			gerentes[i].noVizinho[0] = gerentes[ i^0b0001 ].self;		// FLIP X
			gerentes[i].noVizinho[1] = gerentes[ i^0b0010 ].self;		// FLIP Y
			gerentes[i].noVizinho[2] = gerentes[ i^0b0100 ].self;		// FLIP Z
			gerentes[i].noVizinho[3] = gerentes[ i^0b1000 ].self;		// FLIP W
		}
	} else if( TORUS == topologia ) {
		for( int i = 0; i < 16; i++ ) {
			gerentes[i].nVizinhos = 4;
			gerentes[i].noVizinho = calloc( 4, sizeof(gerente_metadados_t) );
			gerentes[i].noVizinho[0] = gerentes[ (i+12)%16 ].self;						// UP
			gerentes[i].noVizinho[1] = gerentes[ ((i%4)+1)%4 + 4*((int)i/4) ].self;		// RIGHT
			gerentes[i].noVizinho[2] = gerentes[ (i+4)%16 ].self;						// DOWN
			gerentes[i].noVizinho[3] = gerentes[ ((i%4)+3)%4 + 4*((int)i/4) ].self;		// LEFT
		}
	} else { // "Fat" Tree
		//                   00
		//        01                    02
		//   03        04          05        06
		// 07  08    09  10      11  12    13  14
		gerentes[ 00 ].nVizinhos = 2;
		gerentes[ 00 ].noVizinho = calloc( 2, sizeof(gerente_metadados_t) );
		gerentes[ 00 ].noVizinho[ 0 ] = gerentes[ 01 ].self;
		gerentes[ 00 ].noVizinho[ 1 ] = gerentes[ 02 ].self;
		gerentes[ 01 ].nVizinhos = 3;
		gerentes[ 01 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
		gerentes[ 01 ].noVizinho[ 0 ] = gerentes[ 00 ].self;
		gerentes[ 01 ].noVizinho[ 1 ] = gerentes[ 03 ].self;
		gerentes[ 01 ].noVizinho[ 2 ] = gerentes[ 04 ].self;
		gerentes[ 02 ].nVizinhos = 3;
		gerentes[ 02 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
		gerentes[ 02 ].noVizinho[ 0 ] = gerentes[ 00 ].self;
		gerentes[ 02 ].noVizinho[ 1 ] = gerentes[ 05 ].self;
		gerentes[ 02 ].noVizinho[ 2 ] = gerentes[ 06 ].self;
		gerentes[ 03 ].nVizinhos = 3;
		gerentes[ 03 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
		gerentes[ 03 ].noVizinho[ 0 ] = gerentes[ 01 ].self;
		gerentes[ 03 ].noVizinho[ 1 ] = gerentes[ 07 ].self;
		gerentes[ 03 ].noVizinho[ 2 ] = gerentes[ 08 ].self;
		gerentes[ 04 ].nVizinhos = 3;
		gerentes[ 04 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
		gerentes[ 04 ].noVizinho[ 0 ] = gerentes[ 01 ].self;
		gerentes[ 04 ].noVizinho[ 1 ] = gerentes[ 09 ].self;
		gerentes[ 04 ].noVizinho[ 2 ] = gerentes[ 10 ].self;
		gerentes[ 05 ].nVizinhos = 3;
		gerentes[ 05 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
		gerentes[ 05 ].noVizinho[ 0 ] = gerentes[ 02 ].self;
		gerentes[ 05 ].noVizinho[ 1 ] = gerentes[ 11 ].self;
		gerentes[ 05 ].noVizinho[ 2 ] = gerentes[ 12 ].self;
		gerentes[ 06 ].nVizinhos = 3;
		gerentes[ 06 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
		gerentes[ 06 ].noVizinho[ 0 ] = gerentes[ 02 ].self;
		gerentes[ 06 ].noVizinho[ 1 ] = gerentes[ 13 ].self;
		gerentes[ 06 ].noVizinho[ 2 ] = gerentes[ 14 ].self;
		gerentes[ 07 ].nVizinhos = 1;
		gerentes[ 07 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 07 ].noVizinho[ 0 ] = gerentes[ 03 ].self;
		gerentes[ 08 ].nVizinhos = 1;
		gerentes[ 08 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 08 ].noVizinho[ 0 ] = gerentes[ 03 ].self;
		gerentes[ 09 ].nVizinhos = 1;
		gerentes[ 09 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 09 ].noVizinho[ 0 ] = gerentes[ 04 ].self;
		gerentes[ 10 ].nVizinhos = 1;
		gerentes[ 10 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 10 ].noVizinho[ 0 ] = gerentes[ 04 ].self;
		gerentes[ 11 ].nVizinhos = 1;
		gerentes[ 11 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 11 ].noVizinho[ 0 ] = gerentes[ 05 ].self;
		gerentes[ 12 ].nVizinhos = 1;
		gerentes[ 12 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 12 ].noVizinho[ 0 ] = gerentes[ 05 ].self;
		gerentes[ 13 ].nVizinhos = 1;
		gerentes[ 13 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 13 ].noVizinho[ 0 ] = gerentes[ 06 ].self;
		gerentes[ 14 ].nVizinhos = 1;
		gerentes[ 14 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
		gerentes[ 14 ].noVizinho[ 0 ] = gerentes[ 06 ].self;
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
