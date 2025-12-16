# Flagi kompilatora (i linkera!)
override CFLAGS=-std=c17 -Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-unused-const-variable -g -O0 -fsanitize=address,undefined,leak

# Nazwa pliku wynikowego (zmienilem na main, zeby pasowalo do Twoich przyzwyczajen)
NAME=main

.PHONY: clean all

all: $(NAME)

# Znajdz wszystkie pliki .c w biezacym katalogu
SOURCES=$(wildcard *.c)
# Zamien .c na .o
OBJECTS=$(SOURCES:.c=.o)

# KROK 1: Kompilacja .c -> .o (make robi to automatycznie, ale uzywajac naszych CFLAGS)

# KROK 2: Linkowanie .o -> plik wykonywalny
$(NAME): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(NAME) $(OBJECTS)