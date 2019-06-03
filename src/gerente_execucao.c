#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

#include "../include/gerente_execucao.h"

// msqKey e msqID do escalonador (para uso em função que será chamada somente pelo escalonador)
extern key_t escalonadorMsqKey;
extern int escalonadorMsqID;

/**
 * @brief Armazena as informações locais relativas à cada job a ser executado
 */
typedef struct {
  int jobID; /**< ID do job informado pelo escalonador */
  pid_t procPID; /**< PID do processo em que ele está executando */
  char program[64]; /**< Nome do programa a ser executado */
  time_t start; /**< Segundos desde o epoch de quando o job começou a ser executado */
  time_t end; /**< Segundos desde o epoch de quando o job terminou sua execução */
} job_t;

/**
 * @brief Buffer de mensagens entre gerentes de processo
 */
struct msgBuf {
  long mtype; /**< Tipo da mensagem, podendo ser MSG_START ou MSG_END */
  char mtext[64]; /**< Conteúdo da mensagem */
} msg; // Já cria uma variável global desse tipo

gerente_init_t dados; // Dados específicos à esse gerente_execução relevantes localmente
job_t jobAtual; // Job que deve ser executado, ou está em execução, ou último job executado

/**
 * @brief Forkeia o processo atual. Filho executa job e pai registra o tempo de quando isso aconteceu
 */
void execJob() {
  if( 0 == (jobAtual.procPID = fork()) ) {
    execl( jobAtual.program, jobAtual.program, (char*) NULL ); // argv[0] tem que ser sempre igual ao nome do programa a ser executado
  } else {
    jobAtual.start = time(NULL);
    dados.self.busy = 1; // inútil para o gerente de execução. Escalonador precisa fazer isso em suas variáveis
  }
}

/**
 * @brief Transforma uma string (recebida por msg) em um job
 * 
 * @param[in] msg a string recebida na mensagem
 * @param[out] job o job recuperado da string
 * @return int o ID (NÃO É O PID) do gerente que enviou a mensagem
 */
int stringToJob( char* msg, job_t* job ) {
  int IDSender;
  sscanf( msg, "%d %s %d", &(job->jobID), job->program, &IDSender );
  return IDSender;
}

/**
 * @brief Encaminha um job para outro gerente de execução
 * 
 * @param[in] msqID ID da fila de mensagem do gerente de execução que vai receber o job
 * @param[in] job o job a ser enviado
 */
void sendJob( int msqID, job_t job ) {
  struct msgBuf newMsg;
  newMsg.mtype = MSG_START;
  sprintf( newMsg.mtext, "%d %s %d", job.jobID, job.program, dados.self.id );
  msgsnd( msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
}

/**
 * @brief Trata o recebimento da solicitação de execução de um job
 * 
 * @param mtext a string com a solicitação de execução de um job
 * 
 * Quando um gerente de execução recebe uma solicitação de execução de um job,
 * ele encaminha para todos os seus nós vizinhos (exceto para quem ele acabou de
 * receber essa mensagem) e se prepara para executar ele. Se ele já tiver
 * recebido esse job antes, a mensagem é completamente ignorada e nem repassada.
 */
void rcvJob( char* mtext ) {
  job_t jobRecebido;
  int IDSender = stringToJob( mtext, &jobRecebido );
  #ifdef DEBUG
  printf( "[GERENTE %d]\tRecebi job %d de %d\n", dados.self.id, jobRecebido.jobID, IDSender );
  #endif // DEBUG
  if( jobRecebido.jobID != jobAtual.jobID ) { // Eu não recebi esse job antes
    for(int i = 0; i < dados.nVizinhos; i++ ) {
      if( dados.noVizinho[i].id == IDSender ) continue; // Envio para todos meus vizinhos, exceto para quem enviou para mim
      sendJob( dados.noVizinho[i].msqID, jobRecebido );
    }
    jobAtual = jobRecebido; // Pode executar
    execJob();
  }
}

/**
 * @brief Registra o término da execução do job pelo processo filho e avisaa o escalonador
 * 
 * A rota escolhida para avisar o escalonador é sempre enviar a mensagem para o vizinho com o menor ID.
 * Se eu for o gerente de menor ID (ou seja, ID == 0), envio para o escalonador.
 */
void end_exec() {
  jobAtual.end = time(NULL); // Registra o tempo do fim
  struct msgBuf newMsg;
  newMsg.mtype = MSG_END;
  sprintf( newMsg.mtext, "%d %d %d %d", jobAtual.jobID, dados.self.id, (int)jobAtual.start, (int)jobAtual.end ); // Prepara mensagem a ser enviada ao escalonador
  if( dados.self.id == 0 ) { // Eu sou o gerente 0
    #ifdef DEBUG
    printf( "[GERENTE %d]\tTerminei de executar o job %d. Enviando mensagem: \"%s\" para o escalonador\n", dados.self.id, jobAtual.jobID, newMsg.mtext );
    #endif // DEBUG
    msgsnd( dados.escalonador.msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
  } else { // Eu não sou o gerente 0
    int vizinhoLowestID = 0;
    int lowestID = 99;
    for( int i = 0; i < dados.nVizinhos; i++ ) { // Procura o vizinho de menor ID
      if( dados.noVizinho[i].id < lowestID ) {
        vizinhoLowestID = i;
        lowestID = dados.noVizinho[i].id;
      }
    }
    #ifdef DEBUG
    printf( "[GERENTE %d]\tTerminei de executar o job %d. Enviando mensagem: \"%s\" para o gerente %d\n", dados.self.id, jobAtual.jobID, newMsg.mtext, lowestID );
    #endif // DEBUG
    msgsnd( dados.noVizinho[vizinhoLowestID].msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
  }
  dados.self.busy = 0;
}

/**
 * @brief Mata o processo filho e morre
 * 
 * @param x unused
 */
void shutdown( int x ) {
  if( dados.self.busy ) kill( jobAtual.procID );
  waitpid( -1, &wstatus, 0 );
  exit(0);
}

/**
 * @brief loop principal de um gerente de execução
 * 
 * @param[in] dadosIniciais vetor com todos os dados iniciais existentes
 * @param[in] myID ID (não é o PID) desse nó
 * 
 * Esse loop seria equivalente à main só que de cada processo gerente de execução.
 * Como a memória é copiada no momento do fork, dadosIniciais só está 100% correto
 * até o elemento myID, portanto, vamos ignorar tudo e pegar somente o desse nó e
 * salvar numa variável global desse processo.
 */
void gerente_loop( gerente_init_t* dadosIniciais, int myID) {
  dados = dadosIniciais[myID];
  free( dadosIniciais );
  dados.self.pid = getpid();
  jobAtual.jobID = -1; // Para indicar que nunca executou nenhum job
  jobAtual.procPID = 0;

  #ifdef DEBUG
  printf( "[GERENTE %d]\tIniciando execucao... Meus vizinhos sao", dados.self.id );
  for(int i = 0; i < dados.nVizinhos; i++ ) {
    printf( " %d", dados.noVizinho[i].id );
  }
  printf( "\n" );
  #endif // DEBUG
  
  while( 1 ) {
    if( -1 < msgrcv( dados.self.msqID, &msg, 64, 0, IPC_NOWAIT ) ) { // Recebi uma mensagem
      if( MSG_START == msg.mtype ) { // Era pedindo pra executar um job
        rcvJob( msg.mtext );
      } else if( MSG_END == msg.mtype ) { // Era avisando o tempo de execução de um job
        if( dados.self.id == 0 ) { // Eu sou o gerente 0
          #ifdef DEBUG
          printf( "[GERENTE %d]\tRecebi mensagem: \"%s\". Encaminhando para o escalonador\n", dados.self.id, msg.mtext );
          #endif // DEBUG
          msgsnd( dados.escalonador.msqID, &msg, sizeof(msg.mtext), IPC_NOWAIT );
        } else { // Eu não sou o gerente 0
          int vizinhoLowestID = 0;
          int lowestID = 99;
          for( int i = 0; i < dados.nVizinhos; i++ ) { // Procura o vizinho de menor ID
            if( dados.noVizinho[i].id < lowestID ) {
              vizinhoLowestID = i;
              lowestID = dados.noVizinho[i].id;
            }
          }
          #ifdef DEBUG
          printf( "[GERENTE %d]\tRecebi mensagem: \"%s\". Encaminhando para o gerente %d\n", dados.self.id, msg.mtext, lowestID );
          #endif // DEBUG
          msgsnd( dados.noVizinho[vizinhoLowestID].msqID, &msg, sizeof(msg.mtext), IPC_NOWAIT );
        }
      }
    }

    int wstatus;
    if( 0 < waitpid( -1, &wstatus, WNOHANG ) ) { // Processo filho encerrou
      end_exec();
    }
  }
}

/**
 * @brief Cria todos os processos gerentes de execução
 * 
 * @param[in] topologia A topologia desejada. Valores possíveis: HYPERCUBE, TORUS e FAT_TREE
 * @return gerente_init_t* Vetor com as metainformações de cada processo criado.
 * 
 * Primeiro todas as metainformações são criadas, depois elas são conectadas entre si e,
 * por último, os processos são criados e essas informações são passadas para eles ao
 * mesmo tempo.
 */
gerente_init_t* cria_gerentes( int topologia ) {
  if( topologia < HYPERCUBE || FAT_TREE < topologia ) { // Topologia desconhecida
    fprintf( stderr, "[ERRO]\tTopologia desconhecida!\nExistentes: 0 - Hypercubo, 1 - Torus, 2 - Fat Tree.\nFornecida: %d\n\n", topologia );
    return NULL;
  }

  gerente_init_t* gerentes;

  gerentes = (gerente_init_t*) calloc(16, sizeof(gerente_init_t)); // Cria vetor de 16 posições

  // Somente o gerente 0 tem acesso às informações sobre o escalonador
  gerentes[0].escalonador.pid = getpid();
  gerentes[0].escalonador.msqKey = escalonadorMsqKey;
  gerentes[0].escalonador.msqID = escalonadorMsqID;

  for( int i = 0; i < 16; i++ ) { // Cria cada um dos gerentes e suas filas de mensagens.
    gerentes[i].self.id = i;
    gerentes[i].self.msqKey = 0x1233 + i;
    gerentes[i].self.msqID = msgget( gerentes[i].self.msqKey, IPC_CREAT|0x1ff );
    if( 0 > gerentes[i].self.msqID ) {
      fprintf( stderr, "[ERRO]\tErro na obtencao de fila para o gerente %d.\n\n", i );
      free( gerentes );
      return NULL;
    }
  }

  if( HYPERCUBE == topologia ) {
    // Bit 3 = Cubo 0 ou Cubo 1 (Eixo W)
    // Bit 2 = Face superior ou Face inferior (Eixo Z)
    // Bit 1 = Face esquerda ou Face direita (Eixo Y)
    // Bit 0 = Face próxima ou Face distante (Eixo X)
    // i.e.:	0000 = Canto próximo esquerdo superior do cubo 0
    // 			1111 = Canto distante direito inferior do cubo 1
    for( int i = 0; i < 16; i++ ) {
      gerentes[i].nVizinhos = 4;
      gerentes[i].noVizinho = calloc( 4, sizeof(gerente_metadados_t) );
      gerentes[i].noVizinho[0] = gerentes[ i^0b0001 ].self;		// FLIP X
      gerentes[i].noVizinho[1] = gerentes[ i^0b0010 ].self;		// FLIP Y
      gerentes[i].noVizinho[2] = gerentes[ i^0b0100 ].self;		// FLIP Z
      gerentes[i].noVizinho[3] = gerentes[ i^0b1000 ].self;		// FLIP W
    }
  } else if( TORUS == topologia ) {
    for( int i = 0; i < 16; i++ ) {
      gerentes[i].nVizinhos = 4;
      gerentes[i].noVizinho = calloc( 4, sizeof(gerente_metadados_t) );
      gerentes[i].noVizinho[0] = gerentes[ (i+12)%16 ].self;						// UP
      gerentes[i].noVizinho[1] = gerentes[ ((i%4)+1)%4 + 4*((int)i/4) ].self;		// RIGHT
      gerentes[i].noVizinho[2] = gerentes[ (i+4)%16 ].self;						// DOWN
      gerentes[i].noVizinho[3] = gerentes[ ((i%4)+3)%4 + 4*((int)i/4) ].self;		// LEFT
    }
  } else { // "Fat" Tree
    //                   00
    //        01                    02
    //   03        04          05        06
    // 07  08    09  10      11  12    13  14
    gerentes[ 0 ].nVizinhos = 2;
    gerentes[ 0 ].noVizinho = calloc( 2, sizeof(gerente_metadados_t) );
    gerentes[ 0 ].noVizinho[ 0 ] = gerentes[ 1 ].self;
    gerentes[ 0 ].noVizinho[ 1 ] = gerentes[ 2 ].self;
    gerentes[ 1 ].nVizinhos = 3;
    gerentes[ 1 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
    gerentes[ 1 ].noVizinho[ 0 ] = gerentes[ 0 ].self;
    gerentes[ 1 ].noVizinho[ 1 ] = gerentes[ 3 ].self;
    gerentes[ 1 ].noVizinho[ 2 ] = gerentes[ 4 ].self;
    gerentes[ 2 ].nVizinhos = 3;
    gerentes[ 2 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
    gerentes[ 2 ].noVizinho[ 0 ] = gerentes[ 0 ].self;
    gerentes[ 2 ].noVizinho[ 1 ] = gerentes[ 5 ].self;
    gerentes[ 2 ].noVizinho[ 2 ] = gerentes[ 6 ].self;
    gerentes[ 3 ].nVizinhos = 3;
    gerentes[ 3 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
    gerentes[ 3 ].noVizinho[ 0 ] = gerentes[ 1 ].self;
    gerentes[ 3 ].noVizinho[ 1 ] = gerentes[ 7 ].self;
    gerentes[ 3 ].noVizinho[ 2 ] = gerentes[ 8 ].self;
    gerentes[ 4 ].nVizinhos = 3;
    gerentes[ 4 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
    gerentes[ 4 ].noVizinho[ 0 ] = gerentes[ 1 ].self;
    gerentes[ 4 ].noVizinho[ 1 ] = gerentes[ 9 ].self;
    gerentes[ 4 ].noVizinho[ 2 ] = gerentes[ 10 ].self;
    gerentes[ 5 ].nVizinhos = 3;
    gerentes[ 5 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
    gerentes[ 5 ].noVizinho[ 0 ] = gerentes[ 2 ].self;
    gerentes[ 5 ].noVizinho[ 1 ] = gerentes[ 11 ].self;
    gerentes[ 5 ].noVizinho[ 2 ] = gerentes[ 12 ].self;
    gerentes[ 6 ].nVizinhos = 3;
    gerentes[ 6 ].noVizinho = calloc( 3, sizeof(gerente_metadados_t) );
    gerentes[ 6 ].noVizinho[ 0 ] = gerentes[ 2 ].self;
    gerentes[ 6 ].noVizinho[ 1 ] = gerentes[ 13 ].self;
    gerentes[ 6 ].noVizinho[ 2 ] = gerentes[ 14 ].self;
    gerentes[ 7 ].nVizinhos = 1;
    gerentes[ 7 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 7 ].noVizinho[ 0 ] = gerentes[ 3 ].self;
    gerentes[ 8 ].nVizinhos = 1;
    gerentes[ 8 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 8 ].noVizinho[ 0 ] = gerentes[ 3 ].self;
    gerentes[ 9 ].nVizinhos = 1;
    gerentes[ 9 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 9 ].noVizinho[ 0 ] = gerentes[ 4 ].self;
    gerentes[ 10 ].nVizinhos = 1;
    gerentes[ 10 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 10 ].noVizinho[ 0 ] = gerentes[ 4 ].self;
    gerentes[ 11 ].nVizinhos = 1;
    gerentes[ 11 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 11 ].noVizinho[ 0 ] = gerentes[ 5 ].self;
    gerentes[ 12 ].nVizinhos = 1;
    gerentes[ 12 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 12 ].noVizinho[ 0 ] = gerentes[ 5 ].self;
    gerentes[ 13 ].nVizinhos = 1;
    gerentes[ 13 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 13 ].noVizinho[ 0 ] = gerentes[ 6 ].self;
    gerentes[ 14 ].nVizinhos = 1;
    gerentes[ 14 ].noVizinho = calloc( 1, sizeof(gerente_metadados_t) );
    gerentes[ 14 ].noVizinho[ 0 ] = gerentes[ 6 ].self;
    gerentes[ 15 ].nVizinhos = 0;
    gerentes[ 15 ].noVizinho = NULL;
  }

  for( int i = 0; i < 16; i++ ) { // Finalmente cria os processos
    if( FAT_TREE == topologia && 15 == i ) break; // Se for FAT_TREE, são somente 15 gerentes, e não 16

    int pid;
    if( 0 == (pid = fork()) ) {
      signal( SIGUSR1, shutdown );
      gerente_loop( gerentes, i );
    } else {
      gerentes[i].self.pid = pid;
    }
  }

  return gerentes;
}

/**
 * @brief Manda uma solicitação de execução para os processos gerentes de execução
 * 
 * @param[in] gerente0_msqID ID da fila de mensagem do gerente 0
 * @param[in] jobID ID do job desejado
 * @param[in] jobProgram Nome do programa desejado
 */
void executar_programa( int gerente0_msqID, int jobID, char* jobProgram ) {
  struct msgBuf newMsg;
  newMsg.mtype = MSG_START;
  sprintf( newMsg.mtext, "%d %s %d", jobID, jobProgram, -1 ); // -1 indica que é escalonador que está mandando a mensagem
  msgsnd( gerente0_msqID, &newMsg, sizeof(newMsg.mtext), IPC_NOWAIT );
}

/**
 * @brief Destrói filas de mensagem e desaloca memória
 * 
 * @param[in] gerentes Lista de informações sobre cada gerente a ser destruída. Variável se torna inutilizável ao final
 */
void destroi_gerentes( gerente_init_t* gerentes ) {
  for(int i = 0; i < 16; i++ ) {
    kill( gerentes[i].self.pid, SIGUSR1 );
    msgctl( gerentes[i].self.msqID, IPC_RMID, NULL ); // Destrói fila de mensagem
    free( gerentes[i].noVizinho ); // Desaloca lista de vizinhos
  }
  free( gerentes ); // Desaloca vetor de gerentes
}
