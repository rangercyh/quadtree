.PHONY: all test clean quadtree uniform_grid loose_tight_double_grid
all: quadtree uniform_grid

CFLAGS= -g3 -std=c99 -O2 -rdynamic -Wall -fPIC -shared -Wno-missing-braces
CPPFLAGS= -g3 -O2 -rdynamic -Wall -fPIC -shared -Wno-missing-braces

IntListInclude=./int_list
SmallListInclude=./small_list

quadtree: quadtree.so
quadtree.so: quadtree/luabinding.c quadtree/Quadtree.c int_list/IntList.c
	gcc $(CFLAGS) -I$(IntListInclude) -o $@ $^

uniform_grid: uniform_grid.so
uniform_grid.so: uniform_grid/UGrid.cpp
	g++ $(CPPFLAGS) -I$(SmallListInclude) -o $@ $^

loose_tight_double_grid: loose_tight_double_grid.so
loose_tight_double_grid.so: loose_tight_double_grid/LGrid.cpp
	g++ $(CPPFLAGS) -I$(SmallListInclude) -o $@ $^

all: test
test:
	lua test.lua

clean:
	rm -f *.so
