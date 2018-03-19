unhdcpy: decompress.c
	gcc decompress.c -o unhdcpy -O3

clean:
	rm -f unhdcpy
