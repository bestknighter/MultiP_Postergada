#include <stdlib.h>
#include <time.h>
#include <sys/types.h>

#include "gerente_execucao.h"

time_t start, end;
pid_t proc, me;

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

void gerente_loop() {
	me = getpid();
	while( 1 ) {
		// verifica se tem msgs pra receber, se sim
			// se tiver que encaminhar alguma msg, faça
			// se tiver de executar algum programa
				// exec();

		// verifica se terminou de executar, se sim
			// end_exec();
	}
}
