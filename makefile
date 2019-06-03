
all: 
	gcc teste.c -o teste
	gcc ./src/main.c ./src/gerente_execucao.c -o main -Wall

debug: 
	gcc teste.c -o teste
	gcc ./src/main.c ./src/gerente_execucao.c -o main -Wall -ggdb -O0 -DDEBUG
