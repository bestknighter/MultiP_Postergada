#include<signal.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include <unistd.h>

int main() {
    struct mensagem {
        long pid;
        char msg[30];  //colocar para enviar o arquivo executavel
    };
    struct mensagem msg_rcv;
    
    int idFila;
    
    if((idFila = msgget(0x1233, 0x124)) < 0) {
        printf("Erro msgget: %d\n", errno);
        exit(1);
    }
    while(1) {
        /*Recebe mensagem do executa_postergado para executar um processo*/
        printf("Id fila(escalonador): %d\n", idFila);
        printf("antes msgreceive\n");
        msgrcv(idFila, &msg_rcv, sizeof(msg_rcv), 0, 0);
        printf("Mensagem: %s", msg_rcv.msg);
    }   
}