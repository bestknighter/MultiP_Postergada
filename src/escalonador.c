#include "../include/escalonador.h"
#include "../include/gerente_execucao.h"

#include <string.h>

key_t escalonadorMsqKey = 0x1123;
int escalonadorMsqID;

int main( int argc, char *argv[ ] ) {
	int msqid;
	key_t key;
	int topologia = atoi(argv[1]);

	key = 2234;

	if((escalonadorMsqID = msgget(escalonadorMsqKey, IPC_CREAT|0x1ff)) < 0) {
		perror("msgget escalonador");
		exit(1);
	}

	/*msg do executa postergado*/
	if ((msqid = msgget(key, 0666)) < 0) {   //não pode executar antes de criar a fila, ou seja, antes do executa_postergado
		perror("msgget");
		exit(1);
	}

	if ((idsem = semget(0x1223, 1, IPC_CREAT|0x1ff)) < 0) {
    	printf("erro na criacao do sem\n");
     	exit(1);
  }

	gerente_init_t* gerentes_execucao = cria_gerentes(topologia);
	//setar eles como livre

 	// Receive an answer of message type 1.
  while (1) {
		if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0) {
			perror("msgrcv");
			exit(1);
		}
		int tempoEspera;
		int jobID;
		char *nomePrograma;

		nomePrograma=buscarInfoMsgPostergada(&tempoEspera, &jobID, nomePrograma);
		printf("tempoEspera: %d\n", tempoEspera);
		printf("Num job: %d\n", jobID);
		printf("Nome programa: %s\n", nomePrograma);

		sleep(tempoEspera);

		p_sem();
			executa_programa(jobID, nomePrograma); //funcao do gerente
			//esperar a mensagem de todos os processos gerentes para liberar o semaforo
			/*
			while(gerente nao esta livre) {
				msgrcv(recebe mensagem de termino dos processos).
			}
			*/
		v_sem();
	}
	exit(0);
}

char * buscarInfoMsgPostergada(int *tempoEspera, int *jobID, char *nomePrograma) {
	char delim[] = " ";

	char *ptr = strtok(rbuf.mtext, delim);
	*jobID = atoi(ptr);
	printf("ptr1: %d\n", *jobID);

	nomePrograma = strtok(NULL, delim);
	printf("ptr2: %s\n", nomePrograma);

	ptr = strtok(NULL, delim);
	*tempoEspera = atoi(ptr);
	printf("ptr3: %d\n", *tempoEspera);
   return nomePrograma;
}

void executa_programa(int jobID, char* nomePrograma) {
	/*
	lock(livre)
		1-solicitar que todos os processos gerentes de execução executem o executavel
		2-marcar todos eles como ocupado
		3-esperar que todos os processos informem que acabou a execucao
		4-marcar processo gerente de execucao como livre.
		imprime informacoes da execucao
	unlock(livre)
	*/
		printf("^%s\n", rbuf.mtext);
}

int p_sem() {
	operacao[0].sem_num = 0;
  operacao[0].sem_op = 0;
  operacao[0].sem_flg = 0;
  operacao[1].sem_num = 0;
  operacao[1].sem_op = 1;
  operacao[1].sem_flg = 0;
  if ( semop(idsem, operacao, 2) < 0)
		printf("erro no p=%d\n", errno);
}

int v_sem() {
	operacao[0].sem_num = 0;
  operacao[0].sem_op = -1;
  operacao[0].sem_flg = 0;
  if ( semop(idsem, operacao, 1) < 0)
  	printf("erro no p=%d\n", errno);
}
