build: tema3
tema3: tema3.c
	mpicc tema3.c -o tema3 -lm
clean:
	rm -f tema3
