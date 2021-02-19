.PHONY: all quadtree test clean
all: quadtree test

CFLAGS= -g3 -std=c99 -O2 -rdynamic -Wall -fPIC -shared -Wno-missing-braces

IntListInclude=./int_list
SmallListInclude=./small_list

quadtree: quadtree.so
quadtree.so: quadtree/luabinding.c quadtree/Quadtree.c int_list/IntList.c
	gcc $(CFLAGS) -I$(IntListInclude) -o $@ $^

test:
	lua test.lua

clean:
	rm -f quadtree.so
