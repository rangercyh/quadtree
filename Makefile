.PHONY : test
all : quadtree.so test

CFLAGS = -g3 -std=c99 -O2 -rdynamic -Wall -fPIC -shared -Wno-missing-braces

quadtree.so: luabinding.c Quadtree.c IntList.c
	gcc $(CFLAGS) -o $@ $^

test:
	lua test.lua

clean :
	rm quadtree.so
