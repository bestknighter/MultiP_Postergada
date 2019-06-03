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
	while(1) {
		if( -1 < msgrcv( escalonadorMsqID, &msg, 64, 0, 0 ) ) {
			#ifdef DEBUG
			printf( "[ESCALONADOR]\tRecebi a mensagem: \"%s\"\n", msg.mtext );
			#endif // DEBUG
		}
	}
	int wstatus;
	waitpid( -1, &wstatus, 0 );
	return 0;
}

void clear( int x ) {
	#ifdef DEBUG
	printf( "[ESCALONADOR]\tLimpando filas e liberando memória...\n" );
	#endif // DEBUG
	
	destroi_gerentes( gerentes );
	msgctl( escalonadorMsqID, IPC_RMID, NULL );
	exit( x );
}

