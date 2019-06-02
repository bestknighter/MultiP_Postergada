#include "../include/gerente_execucao.h"

key_t escalonadorMsqKey;
int escalonadorMsqID;

int main( int argc, char* argv[] ) {
	gerente_init_t* gerentes;
	escalonadorMsqKey = 0x1111;
	escalonadorMsqID = msgget( escalonadorMsqKey, IPC_CREAT|0x1ff );
	gerentes = cria_gerentes( HYPERCUBE );
	int wstatus;
	waitpid( -1, &wstatus );
	return 0;
}