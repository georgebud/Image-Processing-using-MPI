build: homework
tema3: homework.c
	mpicc homework.c -o homework -lm
clean:
	rm -f homework
