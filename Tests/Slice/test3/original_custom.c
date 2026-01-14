int* find_max(int *p, int n) {
    if (n <= 0) return p;
    int *maxp = p; 
    for (int i = 1; i < n; ++i) {
        if (*(p + i) > *maxp) {
            maxp = p + i;   
int main(void) {
    int arr[6] = {3, 9, 1, 7, 5, 2};
    int n = 6;
    int *maxp = find_max(arr, n);
