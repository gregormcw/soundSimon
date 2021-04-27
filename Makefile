
output: main.o
	gcc -g -Wall main.c paUtils.c -o game \
	-I/usr/local/include -L/usr/local/lib \
	-lsndfile -lncurses -lportaudio


main.o: main.c
	gcc -c main.c	

clean:
	rm *.o game
	
