#include "../include/gerente_execucao.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

key_t escalonadorMsqKey;
int escalonadorMsqID;
gerente_init_t* gerentes;

void clear( int x );

int main( int argc, char* argv[] ) {
  // Cria fila de mensagens do escalonador
  escalonadorMsqKey = 0x1111;
  escalonadorMsqID = msgget( escalonadorMsqKey, IPC_CREAT|0x1ff );

  // Cria gerentes de execução
  gerentes = cria_gerentes( HYPERCUBE );

  // Sobrescreve o comportamento do SIGINT somente para o escalonador
  signal( SIGINT, clear );
  
  #ifdef DEBUG
  printf( "[ESCALONADOR]\tEnviando comando em 5s...\n" );
  sleep(5);
  #endif // DEBUG

  // Cria um buffer de mensagem para receber as respostas dos gerentes
  struct { long mtype; char mtext[64]; } msg;
  
  // Manda executarem o programa "./teste" usando o jobID = 1234
  // e espera as respostas dos gerentes (15 se for Fat Tree)
  executar_programa( gerentes[0].self.msqID, 1234, "./teste" );
  int count = 16;
  while( count ) {
    if( -1 < msgrcv( escalonadorMsqID, &msg, 64, 0, 0 ) ) {
      count--;
      #ifdef DEBUG
      printf( "[ESCALONADOR]\tRecebi a mensagem: \"%s\"\n", msg.mtext );
      #endif // DEBUG
    }
  }
  
  // Espera 5s para o terminal ficar legível
  sleep(5);
  
  // Manda executarem o programa "./teste" usando o jobID = 1235
  // e espera as respostas dos gerentes (15 se for Fat Tree)
  executar_programa( gerentes[0].self.msqID, 1235, "./teste" );
  count = 16;
  while( count ) {
    if( -1 < msgrcv( escalonadorMsqID, &msg, 64, 0, 0 ) ) {
      count--;
      #ifdef DEBUG
      printf( "[ESCALONADOR]\tRecebi a mensagem: \"%s\"\n", msg.mtext );
      #endif // DEBUG
    }
  }
  
  int wstatus;
  waitpid( -1, &wstatus, 0 );

  // Encerrando normalmente ou não, precisa limpar a memória
  clear( 0 );

  return 0;
}

// Função para liberar toda a memória
void clear( int x ) {
  // Seria bom chamar aqui a função para exibir as informações armazenadas pelo escalonador (makespan, etc)
  // imprime_infos() ?

  #ifdef DEBUG
  printf( "\n[ESCALONADOR]\tLimpando filas e liberando memória...\n" );
  #endif // DEBUG
  
  destroi_gerentes( gerentes );

  // Destrói própria fila de mensagens
  msgctl( escalonadorMsqID, IPC_RMID, NULL );

  // Encerra com o código da interrupção
  exit( x );
}
