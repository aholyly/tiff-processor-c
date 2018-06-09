all: 
	gcc tiffProcessor.c -o tiffProcessor -std=c11

clean:
	rm tiffProcessor
