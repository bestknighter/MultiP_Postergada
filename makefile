release:
	gcc ./src/teste.c -o teste
	gcc ./src/hello.c -o hello
	gcc ./src/executa_postergado.c -o executa_postergado
	gcc ./src/escalonador.c ./src/gerente_execucao.c -o escalonador

debug:
	gcc ./src/teste.c -o teste
	gcc ./src/hello.c -o hello
	gcc ./src/executa_postergado.c -o executa_postergado -Wall -ggdb -O0 -DDEBUG
	gcc ./src/escalonador.c ./src/gerente_execucao.c -o escalonador -Wall -ggdb -O0 -DDEBUG

clean:
	rm teste
	rm hello
	rm executa_postergado
	rm escalonador
