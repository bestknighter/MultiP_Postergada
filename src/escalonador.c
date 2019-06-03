#include "../include/escalonador.h"
#include "../include/gerente_execucao.h"

#include <string.h>
#include <time.h>

key_t escalonadorMsqKey = 0x1123;
int escalonadorMsqID;

/**
 * @brief Buffer de mensagens entre gerentes de processo
 */
struct msgBuf {
  long mtype; /**< Tipo da mensagem, podendo ser MSG_END */
  char mtext[64]; /**< Conteúdo da mensagem */
} msg; // Já cria uma variável global desse tipo

int main( int argc, char *argv[ ] ) {
  int executaPostergadoMsqID;
  key_t key;
  topologia = atoi(argv[1]);

  key = 2234;

  if((escalonadorMsqID = msgget(escalonadorMsqKey, IPC_CREAT|0x1ff)) < 0) {
    perror("msgget escalonador");
    exit(1);
  }

  /*busca a fila de mensagem do executa postergado*/
  if ((executaPostergadoMsqID = msgget(key, 0666)) < 0) { //não pode executar antes de criar a fila, ou seja, antes do executa_postergado
    perror("msgget");
    exit(1);
  }

  if ((idsem = semget(0x1223, 1, IPC_CREAT|0x1ff)) < 0) {
      printf("erro na criacao do sem\n");
      exit(1);
    }

    int tempoEspera;
    int jobID;
    char *nomePrograma;

    while (1) {
    /*Receive an answer of message type 1.*/
    if (msgrcv(executaPostergadoMsqID, &rbuf, MSGSZ, 1, 0) < 0) {
      perror("msgrcv");
      exit(1);
    }

    printf("tempo espera: %d\n", tempoEspera);
    printf("nome programa: %s\n", nomePrograma);
    printf("jobID: %d\n", jobID);

    nomePrograma=buscar_info(&tempoEspera, &jobID, nomePrograma);
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
      perror("msgrcv mensagem termino");
      exit(1);
    }
    makespan += tempo_execucao(msg.mtext);
  }
  return makespan;
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
    printf("erro no p=%d\n", errno);
}

int v_sem() {
  operacao[0].sem_num = 0;
  operacao[0].sem_op = -1;
  operacao[0].sem_flg = 0;
  if ( semop(idsem, operacao, 1) < 0)
    printf("erro no p=%d\n", errno);
}
