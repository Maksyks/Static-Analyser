int** create_matrix(int rows, int cols) {
    int **m = malloc(rows * sizeof(int*)); 
    for (int i = 0; i < rows; ++i) {
        m[i] = malloc(cols * sizeof(int));
    return m;
void init_matrix(int **m, int rows, int cols) {
    for (int i = 0; i < rows; ++i) {
        int *row = m[i]; 
int main(void) {
    int n = 4;
    int **mat = create_matrix(n, n);
    init_matrix(mat, n, n);
