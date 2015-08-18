# Makefile
# C Hashmap project
obj=hashmap.o hashmap_tests.o
inc=hashmap.h
misc=Makefile
target=hashmap_tests
cflags=-Wall -g -O0 -Werror -pedantic -std=c99

test: all
	./$(target)

valgrind: clean all
	valgrind --show-leak-kinds=all --leak-check=full ./$(target)

all: $(target)

$(target) : $(obj) $(misc)
	gcc $(cflags) -o $(target) $(obj) $(lflags)

%.o : %.c $(misc) $(inc)
	gcc $(cflags) -c -o $@ $<

clean:
	rm -f $(obj) *~ $(target)
