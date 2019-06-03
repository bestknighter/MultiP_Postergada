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
  int msqid;
  key_t key;
  topologia = atoi(argv[1]);

  key = 2234;

  if((escalonadorMsqID = msgget(escalonadorMsqKey, IPC_CREAT|0x1ff)) < 0) {
    perror("msgget escalonador");
    exit(1);
  }

  /*msg do executa postergado*/
  if ((msqid = msgget(key, 0666)) < 0) { //não pode executar antes de criar a fila, ou seja, antes do executa_postergado
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
    if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0) {
      perror("msgrcv");
      exit(1);
    }

    int tempoEspera;
    int jobID;
    char *nomePrograma;

    nomePrograma=buscar_info(&tempoEspera, &jobID, nomePrograma);

    printf("tempo espera: %d\n", tempoEspera);
    printf("nome programa: %s\n", nomePrograma);
    printf("jobID: %d\n", jobID);


    sleep(tempoEspera); //nao necessariamente que vai pegar o semaforo sera o primeiro da fila
    //verificar se o processo é o primeiro da fila(criar uma queue) verifica quem é o primeiro da fila, se for igual ao jobID, pode executar, caso contrario, espera
    /*
    */
    /*
    */
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
    printf("esperando mensagens");
    if(msgrcv(escalonadorMsqID, &msg, sizeof(msg.mtext), MSG_END, 0) < 0) {
      perror("msgrcv mensagem termino");
      exit(1);
    }
    printf("mensagem recebida\n");
    makespan += tempo_execucao(msg.mtext);
    printf("makespan");
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
  printf("Id: %s\n", ptr);
  /*busca o gerenteID*/
  ptr = strtok(NULL, delim);
  printf("GerenteID: %s\n", ptr);
  /*busca o time_t_comeco*/
  ptr = strtok(NULL, delim);
  int totalSegundosInicio = atoi(ptr);
  time_t start = (time_t)totalSegundosInicio;
  
  //printf("Start: %s\n",time_to_string(start));

  /*busca o time_t_fim*/
  ptr = strtok(NULL, delim);
  int totalSegundosFim = atoi(ptr);
  time_t end = (time_t)totalSegundosFim;
  
  //printf("End: %s\n",time_to_string(end));

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
