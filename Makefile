
CC=gcc -std=c99

intermidiator.exe: main.o midi.o utils.o cmdproc.o callback.o signal.o
	$(CC) -o $@ $^ -lwinmm

.c.o:
	$(CC) -c $<

clean:
	rm -f *.o
