#include <stdio.h>
#include <unistd.h>

int main() {
    printf( "Executando o teste! Vou dormir...\n" );
    sleep(3);
    printf( "Acordei e morri!\n" );
    return 0;
}

