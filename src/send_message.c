#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>

/* processo para testar o envio de mensagens. */
int main() {
    struct mensagem {
        long pid;
        char msg[30];
    };
    struct mensagem msg_env;
    
    int idFila = msgget(0x1233, 0x124);
    printf("Id fila(send): %d\n", idFila);
    msg_env.pid = getpid();
    strcpy(msg_env.msg, "arquivo_executavel");
    if(msgsnd(idFila, &msg_env, sizeof(msg_env), 0) < 0) {
        printf("Erro msgsnd: %d\n", errno);
        exit(1);
    }
}