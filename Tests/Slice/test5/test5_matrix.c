#include <stdio.h>
#include <stdlib.h>
//Slice with criterion row:14
int** create_matrix(int rows, int cols) {
    int **m = malloc(rows * sizeof(int*)); 
    for (int i = 0; i < rows; ++i) {
        m[i] = malloc(cols * sizeof(int));
    }
    return m;
}

void init_matrix(int **m, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        int *row = m[i]; 
        for (int j = 0; j < cols; ++j) {
            *(row + j) = i * cols + j;
        }
    }
}

int sum_diag(int **m, int n) {
    int sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += *(*(m + i) + i);
    }
    return sum;
}

int main(void) {
    int n = 4;
    int **mat = create_matrix(n, n);

    init_matrix(mat, n, n);

    int sd = sum_diag(mat, n);
    printf("Sum of diagonal = %d\n", sd);

    for (int i = 0; i < n; ++i) {
        free(mat[i]);
    }
    free(mat); 

    return 0;
}
