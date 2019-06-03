#include "../include/escalonador.h"
#include "../include/gerente_execucao.h"

#include <string.h>
#include <time.h>

key_t escalonadorMsqKey = 0x1123; /**< Variavel global que vai ser usado pelo gerente_execucao como extern */
int escalonadorMsqID; /**< Variavel global que tambem vai ser usada por gerente_execucao */

#define MSGSZ 128

int topologia; /**< Variavel que vai guardar o valor da topologia passada como parametro para o escalonador */
int idsem;
/**
 * @brief Buffer de mensagens entre gerentes de processo e entre
 * executa_postergado
 */
struct msgBuf {
	long mtype; /**< Tipo da mensagem, podendo ser MSG_END */
	char mtext[MSGSZ]; /**< Conteúdo da mensagem */
} msgGerente, msgPost; // Já cria uma variável global desse tipo

int main( int argc, char *argv[ ] ) {
	int executaPostergadoMsqID;
	key_t keyMsgPost;
	topologia = atoi(argv[1]);

	keyMsgPost = 2234;

	if((escalonadorMsqID = msgget(escalonadorMsqKey, IPC_CREAT|0x1ff)) < 0) {
		perror("msgget escalonador");
		exit(1);
	}

	/*busca a fila de mensagem do executa postergado*/
	if ((executaPostergadoMsqID = msgget(keyMsgPost, 0666)) < 0) {   //não pode executar antes de criar a fila, ou seja, antes do executa_postergado
		perror("msgget");
		exit(1);
	}

	if ((idsem = semget(0x1223, 1, IPC_CREAT|0x1ff)) < 0) {
    	printf("erro na criacao do sem\n");
     	exit(1);
    }

	gerente_init_t* gerentes_execucao = cria_gerentes(topologia);

    while (1) {
 		/*Receive an answer of message type 1.*/
		if (msgrcv(executaPostergadoMsqID, &msgPost, sizeof(msgPost.mtext), 1, 0) < 0) {
			perror("msgrcv");
			exit(1);
		}

		int tempoEspera;
		int jobID;
		char *nomePrograma;

		nomePrograma=buscar_info(&tempoEspera, &jobID);

        printf("tempoEspera: %d\n", tempoEspera);
        printf("jobID: %d\n", jobID);
        printf("nomePrograma: %s\n", nomePrograma);

		sleep(tempoEspera);
		p_sem();
			executar_programa(gerentes_execucao[0].self.msqID, jobID, nomePrograma);
			double makespan = esperar_mensagens();	
			printf("job=%d, arquivo=%s, delay=%d segundos, makespan=%f segundos\n", jobID, nomePrograma, tempoEspera, makespan);	
		v_sem();

	}
	exit(0);
}

/**
 * @brief Espera a mensagem de termino de todos os processos gerentes
 * e calcula o makespan desses processos.
 * @return retorna o turnaround(makespan) em segundos  
 */
double esperar_mensagens() {
	double makespan = 0.0;
	int total_proc = (topologia == HYPERCUBE || topologia == TORUS) ? 16 : 15;

	for(int i=0; i < total_proc; i++) {
		if(msgrcv(escalonadorMsqID, &msgGerente, sizeof(msgGerente.mtext), MSG_END, 0) < 0) {
			perror("msgrcv mensagem termino");
			exit(1);
		}
		makespan += tempo_execucao(msgGerente.mtext);
	}
	return makespan;	
}

/**
 * @brief Calcula o tempo de execucao de um processo
 * 
 * @param[in] msg possuindo o formato: int_jobID int_gerenteID time_t_start time_t_end
 * @return diferenca de tempo entre end e start
 */
double tempo_execucao(char *msg) {
	printf("msg before: %s\n", msg);
	char delim[] = " ";

	/*busca o jobID*/
	char *ptr = strtok(msg, delim);
	/*busca o gerenteID*/
	ptr = strtok(NULL, delim);
	/*busca o time_t_comeco*/
	ptr = strtok(NULL, delim);
	int totalSegundosInicio = atoi(ptr);
	time_t start = (time_t)totalSegundosInicio;
	/*busca o time_t_fim*/
	ptr = strtok(NULL, delim);
	int totalSegundosFim = atoi(ptr);
	time_t end = (time_t)totalSegundosFim;

	/*Calculates the difference in seconds between beginning and end  (time1 - time2).*/
	return difftime(end, start);
}

/**
 * @brief Busca o delay, o jobID e o nomePrograma da mensagem recebida
 * do executa_postergado
 * 
 * @param[in] tempoEspera recebe o endereco da variavel que vai guardar o delay
 * @param[in] jobID recebe o endereco da variavel que vai guardar o job
 * @return char * nomePrograma 
 */
char * buscar_info(int *tempoEspera, int *jobID) {
	char delim[] = " ";

	char *ptr = strtok(msgPost.mtext, delim);
	*jobID = atoi(ptr);

	char *nomePrograma = strtok(NULL, delim);

	ptr = strtok(NULL, delim);
	*tempoEspera = atoi(ptr);

   return nomePrograma;
}

/* Obs.: nao esta sendo usada, funcao feita para debugar o codigo. */
char * time_to_string(time_t time) {
	char buff[20];
	struct tm * timeinfo;
	timeinfo = localtime (&time);
	strftime(buff, sizeof(buff), "%b %d %H:%M:%S", timeinfo);
	return buff;
}

/* Semaforo implementado por Alba Melo. */
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
