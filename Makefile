
CC=gcc -std=c99
STRIP=strip
RM=rm -f

intermidiator.exe: main.o midi.o utils.o cmdproc.o callback.o signal.o os_depend.win32.o
	$(CC) -o $@ $^ -lwinmm
	$(STRIP) $@

.c.o:
	$(CC) -c $<

clean:
	$(RM) *.o
