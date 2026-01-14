int sum_array(int arr[], int n) {
    int sum = 0;
    int *p = arr; 
    for (int i = 0; i < n; ++i) {
        sum += *(p + i);
int main(void) {
    int a[5] = {1, 2, 3, 4, 5};
    int result = sum_array(a, 5);
