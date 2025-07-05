CC = g++
CPPFLAGS = -I. -lpthread -lmimalloc -march=native -msse4 -maes -std=c++20 -O3 -DNDEBUG
CPPFLAGS_DEBUG = -I. -lpthread -lmimalloc -march=native -msse4 -maes -std=c++20 -g
EXEC=main

all: $(EXEC)

$(EXEC): src/main.cpp
	$(CC) $< $(CPPFLAGS) -o $(EXEC)

gen:
	@make -C ycsbc gen

clean:
	@$(RM) $(EXEC)

.PHONY: all clean
