# Makefile
# C Hashmap project
main=hashmap_tests.o
src=hashmap.c hashmap_tests.c
obj=hashmap.o hashmap_tests.o
inc=hashmap.h
misc=Makefile
target=hashmap
cflags=-Wall -g -O0 -Werror -pedantic -std=c99

all: $(target)

$(target) : $(obj) $(misc)
	gcc $(cflags) -o $(target) $(obj) $(lflags)

%.o : %.c $(misc) $(inc)
	gcc $(cflags) -c -o $@ $<

clean:
	rm -f $(obj) *~

test: $(all)
	./$(main)
