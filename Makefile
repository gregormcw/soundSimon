
game: main.o game.o paUtils.o
	gcc -g -Wall -o game main.o game.o paUtils.o \
	-I/usr/local/include -L/usr/local/lib \
	-lsndfile -lncurses -lportaudio

main.o: main.c game.h
	gcc -c main.c

game.o: game.c game.h buf.h
	gcc -c game.c

paUtils.o: paUtils.c paUtils.h
	gcc -c paUtils.c

clean:
	rm *.o game
	