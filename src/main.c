#include "../include/gerente_execucao.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

key_t escalonadorMsqKey;
int escalonadorMsqID;
gerente_init_t* gerentes;

void clear( int x );

int main( int argc, char* argv[] ) {
	escalonadorMsqKey = 0x1111;
	escalonadorMsqID = msgget( escalonadorMsqKey, IPC_CREAT|0x1ff );
	gerentes = cria_gerentes( HYPERCUBE );
	signal( SIGINT, clear );
	
	#ifdef DEBUG
	printf( "[ESCALONADOR]\tEnviando comando em 5s...\n" );
	sleep(5);
	#endif // DEBUG
	
	executar_programa( gerentes[0].self.msqID, 1234, "./teste" );
	struct { long mtype; char mtext[64]; } msg;
	int count = 16;
	while( count ) {
		if( -1 < msgrcv( escalonadorMsqID, &msg, 64, 0, 0 ) ) {
			count--;
			#ifdef DEBUG
			printf( "[ESCALONADOR]\tRecebi a mensagem: \"%s\"\n", msg.mtext );
			#endif // DEBUG
		}
	}
	
	sleep(5);
	
	executar_programa( gerentes[0].self.msqID, 1235, "./teste" );
	count = 16;
	while( count ) {
		if( -1 < msgrcv( escalonadorMsqID, &msg, 64, 0, 0 ) ) {
			count--;
			#ifdef DEBUG
			printf( "[ESCALONADOR]\tRecebi a mensagem: \"%s\"\n", msg.mtext );
			#endif // DEBUG
		}
	}
	
	int wstatus;
	waitpid( -1, &wstatus, 0 );

	clear( 0 );

	return 0;
}

void clear(int x) {
	#ifdef DEBUG
	printf( "\n[ESCALONADOR]\tLimpando filas...\n" );
	#endif // DEBUG
	
	for(int i = 0; i < 16; i++ ) {
		msgctl( gerentes[i].self.msqID, IPC_RMID, NULL );
	}
	msgctl( escalonadorMsqID, IPC_RMID, NULL );
	exit( x );
}
