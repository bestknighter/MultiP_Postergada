#include "../include/escalonador.h"
#include "../include/gerente_execucao.h"

#include <string.h>
#include <time.h>
#include <errno.h>

key_t escalonadorMsqKey = 0x1123;
int escalonadorMsqID;
int executaPostergadoMsqID;
gerente_init_t* gerentes_execucao;
int lastJobID;
void clear( int x );

/**
 * @brief Buffer de mensagens entre gerentes de processo
 */
struct msgBuf {
  long mtype; /**< Tipo da mensagem, podendo ser MSG_END */
  char mtext[64]; /**< Conteúdo da mensagem */
} msg; // Já cria uma variável global desse tipo

int main( int argc, char *argv[ ] ) {
  key_t key;
  if( argc != 2 ) {
    fprintf( stderr, "Usage: escalonador <TOPOLOGIA>\n<TOPOLOGIA>:\t0 - Hypercube\n\t\t1 - Torus\n\t\t2 - Fat Tree\n" );
    exit(1);
  }
  topologia = atoi(argv[1]);

  key = 0x2234;

  if((escalonadorMsqID = msgget(escalonadorMsqKey, IPC_CREAT|0x1ff)) < 0) {
    fprintf( stderr, "msgget escalonador\n");
    exit(1);
  }

  /*busca a fila de mensagem do executa postergado*/
  if ((executaPostergadoMsqID = msgget(key, IPC_CREAT|0x1ff)) < 0) { //não pode executar antes de criar a fila, ou seja, antes do executa_postergado
    fprintf( stderr, "msgget: %d\n", errno);
    exit(1);
  }

  if ((idsem = semget(0x1223, 1, IPC_CREAT|0x1ff)) < 0) {
    fprintf( stderr, "erro na criacao do sem\n");
    exit(1);
  }

  int tempoEspera;
  int jobID;
  char *nomePrograma;

  gerentes_execucao = cria_gerentes( topologia );

  // Sobrescreve o comportamento do SIGINT somente para o escalonador
  signal( SIGINT, clear );

  while (1) {
		enviar_mensagem_postergado(&lastJobID);

    /*Receive an answer of message type 1.*/
    if (msgrcv(executaPostergadoMsqID, &rbuf, MSGSZ, 1, 0) < 0) {
      fprintf( stderr, "msgrcv\n");
      exit(1);
    }

    nomePrograma=buscar_info(&tempoEspera, &jobID, nomePrograma);

    printf("tempo espera: %d\n", tempoEspera);
    printf("nome programa: %s\n", nomePrograma);
    printf("jobID: %d\n", jobID);
		lastJobID=jobID;
    sleep(tempoEspera);
    //p_sem();
    executar_programa(gerentes_execucao[0].self.msqID, jobID, nomePrograma);
    double makespan = esperar_mensagens();
    printf("job=%d, arquivo=%s, delay=%d segundos, makespan=%f segundos\n", jobID, nomePrograma, tempoEspera, makespan);
    //v_sem();
  }

  exit(0);
}

/*retorna makespan*/
double esperar_mensagens() {
  double makespan = 0.0;
  int total_proc = (topologia == HYPERCUBE || topologia == TORUS) ? 16 : 15;

  for(int i=0; i < total_proc; i++) {
    if(msgrcv(escalonadorMsqID, &msg, sizeof(msg.mtext), MSG_END, 0) < 0) {
      fprintf( stderr, "msgrcv mensagem termino\n");
      exit(1);
    }
    makespan += tempo_execucao(msg.mtext);
  }
  return makespan;
}
void enviar_mensagem_postergado(int *jobID){
	char bufferJob[50];
	sprintf(bufferJob, "%d", *jobID);

	int msqid;
	int msgflg = IPC_CREAT | 0666;
	key_t key;
	message_buf sbuf;
	size_t buf_length;

	key = 2244;


	if ((msqid = msgget(key, msgflg)) < 0) {
		perror("msgget");
		exit(1);
	}
	else

	// We'll send message type 1
	sbuf.mtype = 1;
	(void) strcpy(sbuf.mtext, bufferJob);

	buf_length = strlen(sbuf.mtext) + 1;

	// Send a message.
	if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0){
		perror("msgsnd");
		exit(1);
	}
	//else
		//printf("Message: \"%s\" Sent\n", sbuf.mtext);

}
/*busca os dados da mensagem recebida de um proc. ger. e calcula a
  diferenca de tempo do inicio e do fim da execucao.*/
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

  /*Calculates the difference in seconds between beginning and end (time1 - time2).*/
  return difftime(end, start);
}

/*Busca o delay, o jobID e o nome do programa do char* recebido
  da mensagem do executa_postergado*/
char * buscar_info(int *tempoEspera, int *jobID, char *nomePrograma) {
  char delim[] = " ";

  char *ptr = strtok(rbuf.mtext, delim);
  *jobID = atoi(ptr);

  nomePrograma = strtok(NULL, delim);

  ptr = strtok(NULL, delim);
  *tempoEspera = atoi(ptr);

   return nomePrograma;
}

char * time_to_string(time_t time) {
  char buff[20];
  struct tm * timeinfo;
  timeinfo = localtime (&time);
  strftime(buff, sizeof(buff), "%b %d %H:%M:%S", timeinfo);
  return buff;
}

int p_sem() {
  operacao[0].sem_num = 0;
  operacao[0].sem_op = 0;
  operacao[0].sem_flg = 0;
  operacao[1].sem_num = 0;
  operacao[1].sem_op = 1;
  operacao[1].sem_flg = 0;
  if ( semop(idsem, operacao, 2) < 0)
    fprintf(stderr, "erro no p=%d\n", errno);
}

int v_sem() {
  operacao[0].sem_num = 0;
  operacao[0].sem_op = -1;
  operacao[0].sem_flg = 0;
  if ( semop(idsem, operacao, 1) < 0)
    fprintf(stderr, "erro no p=%d\n", errno);
}

// Função para liberar toda a memória
void clear( int x ) {
  // Seria bom chamar aqui a função para exibir as informações armazenadas pelo escalonador (makespan, etc)
  // imprime_infos() ?

  #ifdef DEBUG
  printf( "\n[ESCALONADOR]\tLimpando filas e liberando memória...\n" );
  #endif // DEBUG

  destroi_gerentes( gerentes_execucao );

  // Destrói própria fila de mensagens
  msgctl( escalonadorMsqID, IPC_RMID, NULL );
  msgctl( executaPostergadoMsqID, IPC_RMID, NULL );

  // Encerra com o código da interrupção
  exit( x );
}
