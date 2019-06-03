#ifndef GERENTE_EXECUCAO_H
#define GERENTE_EXECUCAO_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define HYPERCUBE 0
#define TORUS 1
#define FAT_TREE 2

/**
 * @brief Metainformações de cada processo
 */
typedef struct {
	int id; /**< ID do nó */
	pid_t pid; /**< PID do processo */
	key_t msqKey; /**< Key da fila de mensagens do processo */
	int msqID; /**< ID da fila de mensagens do processo */
	int busy; /**< Indica se o gerente está executando ou não (útil somente para o escalonador) */
} gerente_metadados_t;

/**
 * @brief Informações relevantes para cada processo
 * 
 * Essa struct armazena as metainformações do processo atual e dos seus vizinhos.
 * Do escalonador também caso o gerente seja de ID == 0.
 */
typedef struct {
	gerente_metadados_t self; /**< Metainformações de si mesmo */
	gerente_metadados_t escalonador; /**< Metainformações do escalonador */
	int nVizinhos; /**< Quantos vizinhos esse nó tem */
	gerente_metadados_t *noVizinho; /**< Metainformações de cada vizinho */
} gerente_init_t;

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
gerente_init_t* cria_gerentes( int topologia );

/**
 * @brief Manda uma solicitação de execução para os processos gerentes de execução
 * 
 * @param[in] gerente0_msqID ID da fila de mensagem do gerente 0
 * @param[in] jobID ID do job desejado
 * @param[in] jobProgram Nome do programa desejado
 */
void executar_programa( int gerente0_msqID, int jobID, char* jobProgram );

/**
 * @brief Destrói filas de mensagem e desaloca memória
 * 
 * @param[in] gerentes Lista de informações sobre cada gerente a ser destruída. Variável se torna inutilizável ao final
 */
void destroi_gerentes( gerente_init_t* gerentes );

#endif // GERENTE_EXECUCAO_H
