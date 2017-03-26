all:
	
	gcc -c *c
	gcc -o Project1 Project1.c

clean:
	rm Project1 *o
