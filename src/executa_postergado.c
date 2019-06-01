#include "../include/executa_postergado.h"

int main( int argc, char *argv[ ] )
{
  int cont;
  int sec = atoi(argv[1]);
  char *arq = argv[2];
  int msqid;
  int msgflg = IPC_CREAT | 0666;
  key_t key;
  message_buf sbuf;
  size_t buf_length;
  char *msg;
  key = 2234;
  char buffer[100];

	(void)fprintf(stderr, "\nmsgget: Calling msgget(%#1x,\\%#o)\n", key, msgflg);

	if ((msqid = msgget(key, msgflg)) < 0) {
		perror("msgget");
		exit(1);
	}
	else
		(void)fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);


  if (can_exec(arq)){
    printf("Arquivo executável válido! Nome do arquivo: %s\n", arq);
    //delay(sec);
    //printf("passaram %d segundos\n", sec);
    addTuple(arq,sec);
    listTuples();

    sprintf(buffer, "%d", sec);
    strcat(arq, " ");
    strcat(arq, buffer);
    sbuf.mtype = 1;
    (void) fprintf(stderr, "msggeet: msgget succeeded: msqid = %d\n", msqid);
    (void) strcpy(sbuf.mtext, arq);
    (void) fprintf(stderr, "msgget: msgget succeeded: msqid = %d\n", msqid);

    buf_length = strlen(sbuf.mtext) + 1;

    // Send a message.
    if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0){
      printf("%d, %ld, %s, %zu\n", msqid, sbuf.mtype, sbuf.mtext, buf_length);
      perror("msgsnd");
      exit(1);
    }
    else
      printf("Mensagem: \"%s\" enviada\n", sbuf.mtext);

    exit(0);
    //printf("%d\n",getpid());
  }
  else{
    printf("Erro: Arquivo executável inválido!\n");
  }

  return 0;
}
// delay nao vai ser usado aqui
// void delay(int number_of_seconds)
// {
//     // Converter em microsegundos (10^6)
//     int milli_seconds = 1000000 * number_of_seconds;
//     clock_t start_time = clock();
//     while (clock() < start_time + milli_seconds);
// }
bool can_exec(const char *file)
{
    struct stat st;

    if (stat(file, &st) < 0)
        return 0;
    if ((st.st_mode & S_IXUSR) != 0)
        return 1;
    return 0;
}
static void listTuples(void) {
    printf("==========\nNumero Job(?) %d\n", tupleCount);
    for (int i = 0; i < tupleCount; ++i)
        printf("   [%s] -> %d\n", tuple[i].strVal, tuple[i].intVal);
    puts("==========");
}

static void addTuple(char *str, int val) {
    printf("Adicionando tupla executável nome '%s', com tempo %d segundos\n", str, val);
    strcpy(tuple[tupleCount].strVal, str);
    tuple[tupleCount++].intVal = val;
}

static void deleteTuple(char *str) {
    int index = 0;
    while (index < tupleCount) {
        if (strcmp(str, tuple[index].strVal) == 0) break;
        ++index;
    }
    if (index == tupleCount) return;

    printf("Deletando tupla executável nome '%s', com tempo %d segundos\n", str, tuple[index].intVal);
    if (index != tupleCount - 1) {
        strcpy(tuple[index].strVal, tuple[tupleCount - 1].strVal);
        tuple[index].intVal = tuple[tupleCount - 1].intVal;
    }
    --tupleCount;
}
