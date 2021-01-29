all : Quadtree.so

Quadtree.so: Quadtree.c IntList.c
	gcc -fPIC -shared $^ -o $@

clean :
	rm Quadtree.so
