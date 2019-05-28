#include<signal.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include <unistd.h>

/*1 executa main, depois o escalonador*/

int main() {
    int idFila, pid1, pid2;

    if((idFila = msgget(0x1233, IPC_CREAT | 0x1FF)) < 0) {
        printf("Erro msgget: %d\n", errno);
        exit(1);
    }

    /*if((pid1 = fork()) == 0) {
        if(execl("escalonador", "escalonador","param1", "param2" ,(char*) 0) < 0) {
            printf("Erro execl: %d\n", errno);
            exit(1);
        }   
    }*/
    /*para testar o envio da mensagem para o escalonador.*/
    /*if((pid2 = fork()) == 0) {
        if(execl("send_message", "send_message","param1", "param2" , (char*) 0) < 0) {
            printf("Erro execl: %d\n", errno);
            exit(1);
        }
    }*/
    //printf("Continuo o codigo do pai.\n");
}