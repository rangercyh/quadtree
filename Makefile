all : quadtree.so

CFLAGS = -g3 -O2 -rdynamic -Wall -fPIC -shared

quadtree.so: luabinding.c Quadtree.c IntList.c
	gcc $(CFLAGS) -o $@ $^

clean :
	rm quadtree.so
