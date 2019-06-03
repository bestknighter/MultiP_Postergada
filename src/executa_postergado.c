#include "../include/executa_postergado.h"

int main( int argc, char *argv[ ] ) {
  if( argc != 3 ) {
    fprintf( stderr, "Usage: executa_postergado <DELAY> <PROGRAM>\n<DELAY>:\tTempo em segundos\n<PROGRAM>:\tNome do programa a ser executado\n" );
    exit(1);
  }

  int cont=1;
  char *ptr;
  long sec = strtol(argv[1], &ptr, 10);
  if(sec!=0){
    char *arq = argv[2];
    int msqid;
    int msgflg = 0666;
    key_t key;
    message_buf sbuf;
    size_t buf_length;
    char msg[80]="";
    key = 0x2234;
    char bufferJob[50]="";
    char bufferSec[50]="";

    //(void)fprintf(stderr, "\nmsgget: Calling msgget(%#1x,\\%#o)\n", key, msgflg);

    if( (msqid = msgget(key, msgflg)) < 0 ) {
      perror("msgget");
      exit(1);
    } //else
      //(void)fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);


    if( can_exec(arq) ) {
      cont= recebe_mensagem_job() + 1;
      printf("Arquivo executável válido! Nome do arquivo: %s\n", arq);
      add_tuple(arq,sec);
      list_tuples();
      sprintf(bufferSec, "%ld", sec);
      sprintf(bufferJob, "%d", cont);
      strcat(msg, bufferJob);
      strcat(msg, " ");
      strcat(msg, arq);
      strcat(msg, " ");
      strcat(msg, bufferSec);
      sbuf.mtype = 1;
      (void) strcpy(sbuf.mtext, msg);
      //(void) fprintf(stderr, "msgget: msgget succeeded: msqid = %d\n", msqid);

      buf_length = strlen(sbuf.mtext) + 1;

      // Send a message.
      if( (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0 ) {
        printf("%d, %ld, %s, %zu\n", msqid, sbuf.mtype, sbuf.mtext, buf_length);
        perror("msgsnd");
        exit(1);
      } else
        printf("Mensagem: \"%s\" enviada\n", sbuf.mtext);

      //printf("%d\n",getpid());
    } else {
      printf("Erro: Arquivo executável inválido!\n");
    }
  } else {
    printf("Tempo de postergação deve ser um inteiro maior que 0");
  }

  return 0;
}
int recebe_mensagem_job(){
  int msqid;
  key_t key;
  message_buf rbuf;
  char *ptr;

  key = 2244;

  if ((msqid = msgget(key, 0666)) < 0) {
    perror("msgget");
    exit(1);
  }

  // Receive an answer of message type 1.
  if (msgrcv(msqid, &rbuf, MSGSZ, 1, 0) < 0) {
    perror("msgrcv");
    exit(1);
  }

  printf("^%s\n", rbuf.mtext);
  long job = strtol(rbuf.mtext, &ptr, 10);

  return job;
}
bool can_exec(const char *file) {
  struct stat st;

  if (stat(file, &st) < 0)
    return 0;
  if ((st.st_mode & S_IXUSR) != 0)
    return 1;
  return 0;
}

static void list_tuples(void) {
  printf("Job=%d, ", tupleCount);
  for (int i = 0; i < tupleCount; ++i)
    printf("arquivo=%s, delay=%d\n", tuple[i].strVal, tuple[i].intVal);
}

static void add_tuple(char *str, int val) {
  printf("Adicionando tupla executável nome '%s', com tempo %d segundos\n", str, val);
  strcpy(tuple[tupleCount].strVal, str);
  tuple[tupleCount++].intVal = val;
}
