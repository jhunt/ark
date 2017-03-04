ark: main.o read.o write.o pack.o unpack.o list.o
	$(CC) -o $@ $+

clean:
	rm -f ark *.o
