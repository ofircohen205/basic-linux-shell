ex2: ex2.o
	gcc -Wall ex2.o -o ex2
ex2.o: ex2.c
	gcc -Wall ex2.c -c
clear: 
	rm ex2 ex2.o
