#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define N_ARGS 2

void print_error(char *error) {
	fprintf(stderr, error);
	exit(EXIT_FAILURE);
}

int main(int argc, char const *argv[]) {
	
	FILE *sudoku_file;
	int squarelen = 0, len = 0;
	int **matrix = NULL;

	if(argc != N_ARGS) {
		char error[64];
		sprintf(error, "Usage: %s filename\n", argv[0]);
		print_error(error);
	}

	// Opens file
	sudoku_file = fopen(argv[1], "r");
	if(sudoku_file == NULL) print_error("Could not open file\n");

	// Scans first line (aka the square size)
	if(fscanf(sudoku_file, "%d", &squarelen) == EOF) print_error("Could not read file\n");
	len = squarelen*squarelen;

	// Allocate matrix
	matrix = (int **)malloc(len * sizeof(int*));
	if(matrix == NULL) print_error("Could not allocate space\n");
	for(int i = 0; i < len; i++) {
		matrix[i] = (int *)malloc(len * sizeof(int));
		if(matrix[i] == NULL) print_error("Could not allocate space\n");
	}

	// Read the file
	for(int i = 0; i < len; i++) {
  		for(int j = 0; j < len; j++) {
    		fscanf(sudoku_file, "%d", *(matrix + i) + j);
  			printf("%d ", matrix[i][j]);
  		}
		printf("\n");
	}

	fclose(sudoku_file);
	return 0;
}