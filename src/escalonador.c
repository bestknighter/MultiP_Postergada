#include "../include/escalonador.h"
#include "../include/gerente_execucao.h"

int main( int argc, char *argv[ ] ) {
	int msqid;
	key_t key;
	int topologia = atoi(argv[1]);

	key = 2234;

	if ((msqid = msgget(key, 0666)) < 0) {   //não pode executar antes de criar a fila, ou seja, antes do executa_postergado
		printf("erro na criacao\n");
		perror("msgget");
		exit(1);
	}

	if ((idsem = semget(0x1223, 1, IPC_CREAT|0x1ff)) < 0) {
    	printf("erro na criacao do sem\n");
     	exit(1);
  }

	gerentes_execucao = cria_gerentes(topologia);
	//setar eles como livre

 	// Receive an answer of message type 1.
  while (1) {
		if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0) {
			perror("msgrcv");
			exit(1);
		}
		//pegar o tempo da mensagem do rbuf
		delay(5);
		executa_programa(&rbuf);
	}
	exit(0);
}

void executa_programa() {
	/*
	lock(livre)
		1-solicitar que todos os processos gerentes de execução executem o executavel
		2-marcar todos eles como ocupado
		3-esperar que todos os processos informem que acabou a execucao
		4-marcar processo gerente de execucao como livre.
		imprime informacoes da execucao
	unlock(livre)
	*/
	p_sem();
		printf("^%s\n", rbuf.mtext);
		//verificar se os processos gerentes estão livres.
		//solicita que todos os processos gerentes executem o programa.
	v_sem();
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

void delay(int number_of_seconds) {
    // Converter em microsegundos (10^6)
    int milli_seconds = 1000000 * number_of_seconds;
    clock_t start_time = clock();
    while (clock() < start_time + milli_seconds);
}