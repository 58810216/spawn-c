all:
	gcc -g -c spawn.c -o spawn.o
	gcc -g -c test.c -o test.o 
	gcc -g test.o spawn.o

clean:
	rm -f *.o
	rm -f *.s
	rm -f a.out
