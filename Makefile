CC = gcc
SRC = main.c
TRGT = main
CFLAG = -Wall -Wextra


build: $(SRC)
	$(CC) $(SRC) $(CFLAG) -o $(TRGT)

run: build
	./$(TRGT)

clean:
	rm -vf $(TRGT)
