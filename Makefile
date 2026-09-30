CC     = gcc
CFLAGS = -Wall -Wextra -g -pthread

SRC    = src/main.c src/vec.c src/lexer.c src/parser.c src/executor.c \
         src/builtins.c src/jobs.c src/monitor.c src/signals.c src/memstat.c

OUT    = EduShell

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)

memcheck: $(OUT)
	valgrind --leak-check=full --show-leak-kinds=all ./$(OUT)

.PHONY: all clean memcheck
