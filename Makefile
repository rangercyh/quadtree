all : Quadtree.so

Quadtree.so: Quadtree.c IntList.c
	gcc $^ -o $@

clean :
	rm Quadtree.so
