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
	// Cria fila de mensagens do escalonador
	escalonadorMsqKey = 0x1111;
	escalonadorMsqID = msgget( escalonadorMsqKey, IPC_CREAT|0x1ff );

	// Cria gerentes de execução
	gerentes = cria_gerentes( HYPERCUBE );

	// Sobrescreve o comportamento do SIGINT somente para o escalonador
	signal( SIGINT, clear );
	
	#ifdef DEBUG
	printf( "[ESCALONADOR]\tEnviando comando em 5s...\n" );
	sleep(5);
	#endif // DEBUG
	
	// Manda executarem o programa "./teste" usando o jobID = 1234
	executar_programa( gerentes[0].self.msqID, 1234, "./teste" );

	// Cria um buffer de mensagem para receber as respostas dos gerentes
	struct { long mtype; char mtext[64]; } msg;

	while(1) { // Fica eternamente fazendo o pooling de mensagens recebidas
		if( -1 < msgrcv( escalonadorMsqID, &msg, 64, 0, 0 ) ) { // Recebi uma mensagem
			#ifdef DEBUG
			printf( "[ESCALONADOR]\tRecebi a mensagem: \"%s\"\n", msg.mtext );
			#endif // DEBUG
		}
	}

	// Espera processos filhos encerrarem (mas nunca chega aqui, já que ali em cima é um while(true))
	int wstatus;
	waitpid( -1, &wstatus, 0 );
	return 0;
}

// Função para liberar toda a memória
void clear( int x ) {
	// Seria bom chamar aqui a função para exibir as informações armazenadas pelo escalonador (makespan, etc)
	// imprime_infos() ?

	#ifdef DEBUG
	printf( "[ESCALONADOR]\tLimpando filas e liberando memória...\n" );
	#endif // DEBUG
	
	destroi_gerentes( gerentes );

	// Destrói própria fila de mensagens
	msgctl( escalonadorMsqID, IPC_RMID, NULL );

	// Encerra com o código da interrupção
	exit( x );
}

