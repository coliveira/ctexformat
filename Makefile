CFLAGS = -Wall
PRG = ctexfmt
OBJ = main.o

$(PRG) : $(OBJ)
			  gcc -o $(PRG) $(OBJ)

clean:
	rm -f *.o $(PRG)
